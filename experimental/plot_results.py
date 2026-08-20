"""Create report-ready PNG charts from experimental/results/summary.csv."""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DISPLAY_NAMES = {
    "rle": "RLE",
    "huff": "Huffman",
    "lzw": "LZW",
    "arith": "Arithmetic",
}
COLORS = {
    "rle": "#2563eb",
    "huff": "#dc2626",
    "lzw": "#16a34a",
    "arith": "#9333ea",
}


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def line_chart(rows: list[dict[str, str]], metric: str, ylabel: str, path: Path) -> None:
    import matplotlib.pyplot as plt

    figure, axis = plt.subplots(figsize=(8, 5))
    algorithms = sorted({row["algorithm"] for row in rows})
    for algorithm in algorithms:
        selected = sorted(
            (row for row in rows if row["algorithm"] == algorithm),
            key=lambda row: int(row["original_bytes"]),
        )
        x_values = [int(row["original_bytes"]) / 1024 for row in selected]
        y_values = [float(row[metric]) for row in selected]
        axis.plot(
            x_values,
            y_values,
            marker="o",
            linewidth=2,
            label=DISPLAY_NAMES[algorithm],
            color=COLORS[algorithm],
        )
    axis.set_xscale("log")
    axis.set_xlabel("Input size (KiB, log scale)")
    axis.set_ylabel(ylabel)
    axis.grid(True, which="both", alpha=0.25)
    axis.legend()
    figure.tight_layout()
    figure.savefig(path, dpi=200)
    plt.close(figure)


def bar_chart(rows: list[dict[str, str]], metric: str, ylabel: str, path: Path) -> None:
    import matplotlib.pyplot as plt

    data_types = []
    for row in rows:
        if row["data_type"] not in data_types:
            data_types.append(row["data_type"])
    algorithms = sorted({row["algorithm"] for row in rows})
    width = 0.8 / len(algorithms)
    x_positions = list(range(len(data_types)))

    figure, axis = plt.subplots(figsize=(9, 5))
    lookup = {(row["data_type"], row["algorithm"]): row for row in rows}
    for index, algorithm in enumerate(algorithms):
        offset = (index - (len(algorithms) - 1) / 2) * width
        values = [float(lookup[(name, algorithm)][metric]) for name in data_types]
        axis.bar(
            [position + offset for position in x_positions],
            values,
            width=width,
            label=DISPLAY_NAMES[algorithm],
            color=COLORS[algorithm],
        )
    axis.set_xticks(x_positions, data_types)
    axis.set_ylabel(ylabel)
    axis.grid(True, axis="y", alpha=0.25)
    axis.legend()
    figure.tight_layout()
    figure.savefig(path, dpi=200)
    plt.close(figure)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--summary",
        type=Path,
        default=PROJECT_ROOT / "experimental" / "results" / "summary.csv",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=PROJECT_ROOT / "experimental" / "results" / "charts",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        import matplotlib  # noqa: F401
    except ImportError as error:
        print(f"Matplotlib import failed in: {sys.executable}")
        print(f"Original error: {error}")
        print(f'Install it for this interpreter with: "{sys.executable}" -m pip install matplotlib')
        return 1

    rows = load_rows(args.summary.resolve())
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    size_rows = [row for row in rows if row["scenario"] == "size"]
    entropy_rows = [row for row in rows if row["scenario"] == "entropy"]

    if size_rows:
        line_chart(
            size_rows,
            "compression_ms_median",
            "Median compression time (ms)",
            output_dir / "scenario1_time.png",
        )
        line_chart(
            size_rows,
            "compression_ratio",
            "Compression ratio (x)",
            output_dir / "scenario1_ratio.png",
        )
    if entropy_rows:
        bar_chart(
            entropy_rows,
            "compression_ms_median",
            "Median compression time (ms)",
            output_dir / "scenario2_time.png",
        )
        bar_chart(
            entropy_rows,
            "compression_ratio",
            "Compression ratio (x)",
            output_dir / "scenario2_ratio.png",
        )
    print(f"Charts written to: {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
