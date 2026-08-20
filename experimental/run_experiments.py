"""Benchmark all compression algorithms and export report-ready results.

The script uses the compressor's own reported execution time, independently
records end-to-end wall time, verifies every decompression with SHA-256, and
exports raw trials, aggregated statistics, a Markdown report table, and run
metadata.  Only the Python standard library is required.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import platform
import random
import re
import statistics
import subprocess
import sys
import time
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Sequence


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DATA_DIR = PROJECT_ROOT / "experimental" / "data"
DEFAULT_RESULTS_DIR = PROJECT_ROOT / "experimental" / "results"
DEFAULT_ARTIFACTS_DIR = PROJECT_ROOT / "experimental" / "artifacts"
ALGORITHMS = ("rle", "huff", "lzw", "arith")
DISPLAY_NAMES = {
    "rle": "RLE",
    "huff": "Huffman",
    "lzw": "LZW",
    "arith": "Arithmetic",
}
TIME_PATTERN = re.compile(r"Execution Time:\s*([0-9]+(?:\.[0-9]+)?)\s*ms")

RAW_FIELDS = [
    "scenario",
    "case",
    "size_label",
    "data_type",
    "entropy_bits_per_byte",
    "algorithm",
    "trial",
    "original_bytes",
    "compressed_bytes",
    "compression_execution_ms",
    "compression_wall_ms",
    "decompression_execution_ms",
    "decompression_wall_ms",
    "compression_ratio",
    "space_savings_percent",
    "verified_lossless",
]

SUMMARY_FIELDS = [
    "scenario",
    "case",
    "size_label",
    "data_type",
    "entropy_bits_per_byte",
    "algorithm",
    "trials",
    "original_bytes",
    "compressed_bytes",
    "compression_ratio",
    "space_savings_percent",
    "compression_ms_median",
    "compression_ms_mean",
    "compression_ms_stdev",
    "compression_ms_min",
    "compression_ms_max",
    "compression_wall_ms_median",
    "decompression_ms_median",
    "decompression_wall_ms_median",
    "compression_mib_per_second",
    "verified_lossless",
]


class BenchmarkError(RuntimeError):
    """Raised when the benchmark cannot produce a trustworthy measurement."""


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_reported_time(stdout: str, command: Sequence[str]) -> float:
    match = TIME_PATTERN.search(stdout)
    if not match:
        raise BenchmarkError(
            "Could not parse 'Execution Time' from compressor output.\n"
            f"Command: {' '.join(command)}\nOutput:\n{stdout}"
        )
    return float(match.group(1))


def run_compressor(command: Sequence[str], timeout: float) -> tuple[float, float]:
    started = time.perf_counter_ns()
    try:
        completed = subprocess.run(
            command,
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as error:
        raise BenchmarkError(
            f"Command timed out after {timeout:g} seconds: {' '.join(command)}"
        ) from error
    wall_ms = (time.perf_counter_ns() - started) / 1_000_000.0

    if completed.returncode != 0:
        raise BenchmarkError(
            f"Compressor failed with exit code {completed.returncode}.\n"
            f"Command: {' '.join(command)}\n"
            f"stdout:\n{completed.stdout}\n"
            f"stderr:\n{completed.stderr}"
        )
    return parse_reported_time(completed.stdout, command), wall_ms


def compile_compressor(executable: Path, compiler: str) -> None:
    executable.parent.mkdir(parents=True, exist_ok=True)
    sources = [
        "source/main.cpp",
        "source/rle.cpp",
        "source/huffman.cpp",
        "source/lzw.cpp",
        "source/arithmetic.cpp",
        "source/bit_io.cpp",
        "source/utils.cpp",
    ]
    command = [compiler, *sources, "-o", str(executable), "-std=c++17", "-O2", "-Wall"]
    print("Building compressor:", " ".join(command))
    completed = subprocess.run(
        command,
        cwd=PROJECT_ROOT,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode != 0:
        raise BenchmarkError(f"Build failed with exit code {completed.returncode}.")


def ensure_datasets(data_dir: Path, seed: int, regenerate: bool) -> Path:
    manifest = data_dir / "manifest.csv"
    if regenerate or not manifest.is_file():
        from gen_test_data import generate

        print(f"Generating deterministic datasets (seed={seed})...")
        manifest = generate(data_dir, seed)
    return manifest


def read_cases(manifest: Path, scenarios: set[str]) -> list[dict[str, str]]:
    with manifest.open(newline="", encoding="utf-8") as stream:
        rows = [row for row in csv.DictReader(stream) if row["scenario"] in scenarios]
    if not rows:
        raise BenchmarkError(f"No matching cases found in {manifest}.")
    for row in rows:
        path = PROJECT_ROOT / row["path"]
        if not path.is_file():
            raise BenchmarkError(f"Dataset listed in manifest does not exist: {path}")
        actual_size = path.stat().st_size
        if actual_size != int(row["bytes"]):
            raise BenchmarkError(
                f"Dataset size mismatch for {path}: expected {row['bytes']}, got {actual_size}."
            )
        if sha256_file(path) != row["sha256"]:
            raise BenchmarkError(f"Dataset checksum mismatch: {path}")
    return rows


def compressor_command(
    executable: Path, algorithm: str, mode: str, input_path: Path, output_path: Path
) -> list[str]:
    return [
        str(executable),
        "-a",
        algorithm,
        "-m",
        mode,
        "-i",
        str(input_path),
        "-o",
        str(output_path),
    ]


def run_pair(
    executable: Path,
    algorithm: str,
    input_path: Path,
    compressed_path: Path,
    restored_path: Path,
    timeout: float,
) -> tuple[float, float, float, float, int]:
    compression = compressor_command(
        executable, algorithm, "c", input_path, compressed_path
    )
    compression_ms, compression_wall_ms = run_compressor(compression, timeout)

    decompression = compressor_command(
        executable, algorithm, "d", compressed_path, restored_path
    )
    decompression_ms, decompression_wall_ms = run_compressor(decompression, timeout)

    if sha256_file(input_path) != sha256_file(restored_path):
        raise BenchmarkError(
            f"Lossless verification failed for {algorithm} on {input_path.name}."
        )
    return (
        compression_ms,
        compression_wall_ms,
        decompression_ms,
        decompression_wall_ms,
        compressed_path.stat().st_size,
    )


def benchmark(
    executable: Path,
    cases: Sequence[dict[str, str]],
    algorithms: Sequence[str],
    repeats: int,
    warmups: int,
    timeout: float,
    artifacts_dir: Path,
    shuffle_seed: int,
) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    rng = random.Random(shuffle_seed)
    jobs = [(case, algorithm) for case in cases for algorithm in algorithms]
    rng.shuffle(jobs)

    for job_index, (case, algorithm) in enumerate(jobs, start=1):
        input_path = PROJECT_ROOT / case["path"]
        case_dir = artifacts_dir / algorithm / case["scenario"] / case["case"]
        case_dir.mkdir(parents=True, exist_ok=True)
        compressed_path = case_dir / f"{case['case']}.{algorithm}.bin"
        restored_path = case_dir / f"{case['case']}.restored.txt"
        original_size = input_path.stat().st_size

        print(
            f"[{job_index:02d}/{len(jobs):02d}] {case['case']:<20} "
            f"{DISPLAY_NAMES[algorithm]:<10}",
            flush=True,
        )

        for _ in range(warmups):
            run_pair(
                executable,
                algorithm,
                input_path,
                compressed_path,
                restored_path,
                timeout,
            )

        for trial in range(1, repeats + 1):
            (
                compression_ms,
                compression_wall_ms,
                decompression_ms,
                decompression_wall_ms,
                compressed_size,
            ) = run_pair(
                executable,
                algorithm,
                input_path,
                compressed_path,
                restored_path,
                timeout,
            )
            ratio = original_size / compressed_size if compressed_size else math.inf
            savings = (1.0 - compressed_size / original_size) * 100.0
            rows.append(
                {
                    "scenario": case["scenario"],
                    "case": case["case"],
                    "size_label": case["size_label"],
                    "data_type": case["data_type"],
                    "entropy_bits_per_byte": float(case["entropy_bits_per_byte"]),
                    "algorithm": algorithm,
                    "trial": trial,
                    "original_bytes": original_size,
                    "compressed_bytes": compressed_size,
                    "compression_execution_ms": compression_ms,
                    "compression_wall_ms": compression_wall_ms,
                    "decompression_execution_ms": decompression_ms,
                    "decompression_wall_ms": decompression_wall_ms,
                    "compression_ratio": ratio,
                    "space_savings_percent": savings,
                    "verified_lossless": True,
                }
            )
    return rows


def summarize(rows: Sequence[dict[str, object]]) -> list[dict[str, object]]:
    groups: dict[tuple[str, str, str], list[dict[str, object]]] = defaultdict(list)
    for row in rows:
        key = (str(row["scenario"]), str(row["case"]), str(row["algorithm"]))
        groups[key].append(row)

    result: list[dict[str, object]] = []
    for key in sorted(groups):
        trials = groups[key]
        first = trials[0]
        compression_times = [float(row["compression_execution_ms"]) for row in trials]
        compression_wall = [float(row["compression_wall_ms"]) for row in trials]
        decompression_times = [float(row["decompression_execution_ms"]) for row in trials]
        decompression_wall = [float(row["decompression_wall_ms"]) for row in trials]
        median_ms = statistics.median(compression_times)
        original_bytes = int(first["original_bytes"])

        result.append(
            {
                "scenario": first["scenario"],
                "case": first["case"],
                "size_label": first["size_label"],
                "data_type": first["data_type"],
                "entropy_bits_per_byte": first["entropy_bits_per_byte"],
                "algorithm": first["algorithm"],
                "trials": len(trials),
                "original_bytes": original_bytes,
                "compressed_bytes": first["compressed_bytes"],
                "compression_ratio": first["compression_ratio"],
                "space_savings_percent": first["space_savings_percent"],
                "compression_ms_median": median_ms,
                "compression_ms_mean": statistics.fmean(compression_times),
                "compression_ms_stdev": statistics.stdev(compression_times)
                if len(compression_times) > 1
                else 0.0,
                "compression_ms_min": min(compression_times),
                "compression_ms_max": max(compression_times),
                "compression_wall_ms_median": statistics.median(compression_wall),
                "decompression_ms_median": statistics.median(decompression_times),
                "decompression_wall_ms_median": statistics.median(decompression_wall),
                "compression_mib_per_second": (
                    (original_bytes / (1024 * 1024)) / (median_ms / 1000.0)
                    if median_ms > 0
                    else math.inf
                ),
                "verified_lossless": all(bool(row["verified_lossless"]) for row in trials),
            }
        )
    return result


def write_csv(path: Path, rows: Iterable[dict[str, object]], fields: Sequence[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def format_cell(row: dict[str, object] | None) -> str:
    if row is None:
        return "-"
    return (
        f"{float(row['compression_ms_median']):.2f} ms "
        f"({float(row['compression_ratio']):.2f}x)"
    )


def write_report_table(
    path: Path,
    summary: Sequence[dict[str, object]],
    algorithms: Sequence[str],
    repeats: int,
    warmups: int,
) -> None:
    lookup = {
        (str(row["scenario"]), str(row["case"]), str(row["algorithm"])): row
        for row in summary
    }
    cases: dict[str, dict[str, dict[str, object]]] = {}
    for row in summary:
        cases.setdefault(str(row["scenario"]), {}).setdefault(str(row["case"]), row)

    lines = [
        "# Experimental results",
        "",
        f"Each value is the median of {repeats} measured run(s) after {warmups} warm-up run(s).",
        "The cell format is `compression time (original size / compressed size)`.",
        "Every measured run was decompressed and verified lossless with SHA-256.",
        "",
    ]
    titles = {
        "size": "Scenario 1 - Impact of file size",
        "entropy": "Scenario 2 - Impact of data entropy",
    }
    for scenario in ("size", "entropy"):
        if scenario not in cases:
            continue
        lines.extend(
            [
                f"## {titles[scenario]}",
                "",
                "| File Size | Data Type | "
                + " | ".join(DISPLAY_NAMES[algo] for algo in algorithms)
                + " |",
                "|---:|:---|" + "---:|" * len(algorithms),
            ]
        )
        ordered_cases = sorted(
            cases[scenario].items(), key=lambda item: int(item[1]["original_bytes"])
        )
        for case_name, representative in ordered_cases:
            cells = [
                format_cell(lookup.get((scenario, case_name, algorithm)))
                for algorithm in algorithms
            ]
            lines.append(
                f"| {representative['size_label']} | {representative['data_type']} | "
                + " | ".join(cells)
                + " |"
            )
        lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def executable_version(executable: Path) -> dict[str, object]:
    stat = executable.stat()
    return {
        "path": str(executable),
        "size_bytes": stat.st_size,
        "sha256": sha256_file(executable),
        "modified_utc": datetime.fromtimestamp(stat.st_mtime, timezone.utc).isoformat(),
    }


def parse_args() -> argparse.Namespace:
    default_executable = PROJECT_ROOT / "build" / "compressor.exe"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, default=default_executable)
    parser.add_argument("--data-dir", type=Path, default=DEFAULT_DATA_DIR)
    parser.add_argument("--results-dir", type=Path, default=DEFAULT_RESULTS_DIR)
    parser.add_argument("--artifacts-dir", type=Path, default=DEFAULT_ARTIFACTS_DIR)
    parser.add_argument(
        "--algorithms", nargs="+", choices=ALGORITHMS, default=list(ALGORITHMS)
    )
    parser.add_argument(
        "--scenario", choices=("all", "size", "entropy"), default="all"
    )
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=300.0)
    parser.add_argument("--seed", type=int, default=20260820)
    parser.add_argument(
        "--regenerate-data", action="store_true", help="Overwrite datasets deterministically."
    )
    parser.add_argument(
        "--build", action="store_true", help="Compile the compressor before benchmarking."
    )
    parser.add_argument("--compiler", default="g++")
    return parser.parse_args()


def validate_args(args: argparse.Namespace) -> None:
    if args.repeats < 1:
        raise BenchmarkError("--repeats must be at least 1.")
    if args.warmups < 0:
        raise BenchmarkError("--warmups cannot be negative.")
    if args.timeout <= 0:
        raise BenchmarkError("--timeout must be positive.")


def main() -> int:
    args = parse_args()
    try:
        validate_args(args)
        executable = args.executable.resolve()
        data_dir = args.data_dir.resolve()
        results_dir = args.results_dir.resolve()
        artifacts_dir = args.artifacts_dir.resolve()

        if args.build:
            compile_compressor(executable, args.compiler)
        if not executable.is_file():
            raise BenchmarkError(
                f"Compressor not found: {executable}\n"
                "Build it with 'make' or rerun this script with --build."
            )

        manifest = ensure_datasets(data_dir, args.seed, args.regenerate_data)
        scenarios = {"size", "entropy"} if args.scenario == "all" else {args.scenario}
        cases = read_cases(manifest, scenarios)

        artifacts_dir.mkdir(parents=True, exist_ok=True)
        results_dir.mkdir(parents=True, exist_ok=True)

        started = datetime.now(timezone.utc)
        raw_rows = benchmark(
            executable=executable,
            cases=cases,
            algorithms=args.algorithms,
            repeats=args.repeats,
            warmups=args.warmups,
            timeout=args.timeout,
            artifacts_dir=artifacts_dir,
            shuffle_seed=args.seed + 1000,
        )
        summary_rows = summarize(raw_rows)

        raw_path = results_dir / "raw_results.csv"
        summary_path = results_dir / "summary.csv"
        report_path = results_dir / "report_table.md"
        metadata_path = results_dir / "run_metadata.json"
        write_csv(raw_path, raw_rows, RAW_FIELDS)
        write_csv(summary_path, summary_rows, SUMMARY_FIELDS)
        write_report_table(
            report_path, summary_rows, args.algorithms, args.repeats, args.warmups
        )

        metadata = {
            "started_utc": started.isoformat(),
            "finished_utc": datetime.now(timezone.utc).isoformat(),
            "command": sys.argv,
            "platform": platform.platform(),
            "python": sys.version,
            "cpu_count": os.cpu_count(),
            "seed": args.seed,
            "scenario": args.scenario,
            "algorithms": args.algorithms,
            "repeats": args.repeats,
            "warmups": args.warmups,
            "timeout_seconds": args.timeout,
            "timing_note": (
                "compression_execution_ms and decompression_execution_ms are parsed "
                "from the compressor CLI. Wall-time columns include process startup."
            ),
            "executable": executable_version(executable),
            "manifest": str(manifest),
        }
        metadata_path.write_text(
            json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
        )

        print("\nBenchmark complete.")
        print(f"Raw trials:    {raw_path}")
        print(f"Summary:       {summary_path}")
        print(f"Report table:  {report_path}")
        print(f"Run metadata:  {metadata_path}")
        return 0
    except (BenchmarkError, OSError) as error:
        print(f"Benchmark error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
