"""Generate deterministic ASCII datasets for the compression experiments.

The generated files follow the two scenarios from Project_Compression.pdf:

1. Standard English-like text from 10 KiB to 10 MiB.
2. Repetitive, English-like, and random printable ASCII data at 1 MiB.

All sizes are exact byte counts.  Only the Python standard library is used.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import random
from collections import Counter
from pathlib import Path
from typing import Iterable


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = PROJECT_ROOT / "experimental" / "data"
KIB = 1024
MIB = 1024 * KIB

SIZE_CASES = (
    ("english_10kb", 10 * KIB),
    ("english_100kb", 100 * KIB),
    ("english_1mb", 1 * MIB),
    ("english_10mb", 10 * MIB),
)

# A deliberately broad vocabulary produces English-like data without downloading
# a corpus.  Random sentence construction avoids repeating one short paragraph,
# which would make dictionary algorithms look unrealistically strong.
WORDS = """
the of and to in a is that for it as with was on be by this are from or at an
not have which but has its were their one all can we more also after first new
time data file text algorithm compression input output symbol code dictionary
frequency tree bit byte string program system memory performance result method
model value table size ratio savings execution analysis experiment scenario
standard english random repetitive information computer storage network format
process build read write encode decode measure compare verify report research
example group project practical efficient simple common different each using
when where while because between through during before under over about into
large small medium increase decrease often usually however therefore although
design implementation test correct lossless original compressed restored exact
sequence character pattern distribution entropy probability prefix dynamic
run length huffman lempel ziv welch arithmetic coding source command option
quality reliable meaningful observed expected theoretical complexity linear
heap node leaf stream header payload state range count update current next
real world image archive transmission document language sentence paragraph
students explain present discuss record median average repeated deterministic
""".split()


def human_size(size: int) -> str:
    if size % MIB == 0:
        return f"{size // MIB} MB"
    return f"{size // KIB} KB"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def shannon_entropy(path: Path) -> float:
    counts: Counter[int] = Counter()
    total = 0
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            counts.update(block)
            total += len(block)
    if total == 0:
        return 0.0
    return -sum(
        (count / total) * math.log2(count / total) for count in counts.values()
    )


def write_repeated_pattern(path: Path, size: int, pattern: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    repetitions, remainder = divmod(size, len(pattern))
    with path.open("wb") as stream:
        stream.write(pattern * repetitions)
        stream.write(pattern[:remainder])


def write_random_ascii(path: Path, size: int, seed: int) -> None:
    rng = random.Random(seed)
    alphabet = bytes(range(32, 127))
    path.parent.mkdir(parents=True, exist_ok=True)
    remaining = size
    with path.open("wb") as stream:
        while remaining:
            amount = min(64 * KIB, remaining)
            stream.write(bytes(rng.choice(alphabet) for _ in range(amount)))
            remaining -= amount


def write_english_like(path: Path, size: int, seed: int) -> None:
    rng = random.Random(seed)
    path.parent.mkdir(parents=True, exist_ok=True)
    remaining = size
    sentence_index = 0

    with path.open("wb") as stream:
        while remaining:
            word_count = rng.randint(8, 20)
            words = [rng.choice(WORDS) for _ in range(word_count)]
            words[0] = words[0].capitalize()
            punctuation = rng.choices([".", ",", "?"], weights=[85, 10, 5])[0]
            separator = "\n" if sentence_index % 6 == 5 else " "
            sentence = (" ".join(words) + punctuation + separator).encode("ascii")
            block = sentence[:remaining]
            stream.write(block)
            remaining -= len(block)
            sentence_index += 1


def write_manifest(rows: Iterable[dict[str, object]], output_dir: Path) -> Path:
    manifest = output_dir / "manifest.csv"
    fieldnames = [
        "scenario",
        "case",
        "size_label",
        "data_type",
        "path",
        "bytes",
        "entropy_bits_per_byte",
        "sha256",
    ]
    with manifest.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    return manifest


def generate(output_dir: Path, seed: int) -> Path:
    size_dir = output_dir / "scenario1_size"
    entropy_dir = output_dir / "scenario2_entropy"
    rows: list[dict[str, object]] = []

    for index, (case_name, size) in enumerate(SIZE_CASES):
        path = size_dir / f"{case_name}.txt"
        write_english_like(path, size, seed + index)
        rows.append(make_manifest_row("size", case_name, "Standard English", path, size))

    entropy_size = MIB
    repetitive = entropy_dir / "repetitive_1mb.txt"
    english = entropy_dir / "english_1mb.txt"
    random_path = entropy_dir / "random_1mb.txt"

    # Long runs make this case intentionally low-entropy and RLE-friendly.
    repetitive_pattern = b"A" * 1024 + b"B" * 1024 + b"C" * 1024 + b"D" * 1024
    write_repeated_pattern(repetitive, entropy_size, repetitive_pattern)
    write_english_like(english, entropy_size, seed + 100)
    write_random_ascii(random_path, entropy_size, seed + 200)

    rows.extend(
        [
            make_manifest_row("entropy", "repetitive_1mb", "Highly Repetitive", repetitive, entropy_size),
            make_manifest_row("entropy", "english_1mb", "Standard English", english, entropy_size),
            make_manifest_row("entropy", "random_1mb", "Random Printable ASCII", random_path, entropy_size),
        ]
    )
    return write_manifest(rows, output_dir)


def make_manifest_row(
    scenario: str, case_name: str, data_type: str, path: Path, size: int
) -> dict[str, object]:
    try:
        manifest_path = path.resolve().relative_to(PROJECT_ROOT).as_posix()
    except ValueError:
        manifest_path = str(path.resolve())
    return {
        "scenario": scenario,
        "case": case_name,
        "size_label": human_size(size),
        "data_type": data_type,
        "path": manifest_path,
        "bytes": size,
        "entropy_bits_per_byte": f"{shannon_entropy(path):.6f}",
        "sha256": sha256_file(path),
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help=f"Dataset directory (default: {DEFAULT_OUTPUT_DIR})",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=20260820,
        help="Deterministic random seed (default: 20260820)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    output_dir = args.output_dir.resolve()
    manifest = generate(output_dir, args.seed)
    print(f"Generated benchmark datasets in: {output_dir}")
    print(f"Manifest: {manifest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
