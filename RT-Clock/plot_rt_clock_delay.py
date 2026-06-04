#!/usr/bin/env python3
"""
plot_delay_errors.py — Parse POSIX RT clock delay error logs and produce
an overlapping dot plot + ranked average error table.

Usage:
    python plot_delay_errors.py file1.txt file2.txt [file3.txt ...]

AI USAGE DISCLAIMER:
    I do not take any credit for the code developed herein.
    I generated this code with Claude AI to help me udnerstand the RT Clock code.
    Find the thread here: https://claude.ai/share/e0b5a8cc-74e5-4e66-8f48-80ccdc1fe46d
    Use code with caution.
"""

import sys
import re
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np

# ── Parser ────────────────────────────────────────────────────────────────────

LINE_RE = re.compile(
    r"MY_CLOCK delay error\s*=\s*\d+,\s*nanoseconds\s*=\s*(\d+)"
)

def parse_file(path: str) -> list[int]:
    """Return list of delay error nanosecond values in file order."""
    errors = []
    with open(path, "r") as fh:
        for line in fh:
            m = LINE_RE.search(line)
            if m:
                errors.append(int(m.group(1)))
    if not errors:
        raise ValueError(f"No delay error lines found in '{path}'")
    return errors

# ── Plot ──────────────────────────────────────────────────────────────────────

def plot(datasets: dict[str, list[int]], out_path: str) -> None:
    fig, ax = plt.subplots(figsize=(14, 6))

    colors = cm.tab10(np.linspace(0, 1, len(datasets)))

    for (label, errors), color in zip(datasets.items(), colors):
        indices = list(range(len(errors)))
        ax.scatter(
            indices,
            errors,
            label=label,
            color=color,
            s=18,
            alpha=0.75,
            linewidths=0,
        )

    ax.set_xlabel("Test index", fontsize=12)
    ax.set_ylabel("Delay error (ns)", fontsize=12)
    ax.set_title("MY_CLOCK delay error by test index", fontsize=14)
    ax.legend(title="File", bbox_to_anchor=(1.01, 1), loc="upper left", fontsize=9)
    ax.grid(axis="y", linestyle="--", alpha=0.4)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"\nPlot saved → {out_path}")

# ── Stats table ───────────────────────────────────────────────────────────────

def print_table(datasets: dict[str, list[int]]) -> None:
    rows = []
    for label, errors in datasets.items():
        arr = np.array(errors, dtype=float)
        rows.append({
            "file":    label,
            "n":       len(arr),
            "mean_ns": arr.mean(),
            "min_ns":  arr.min(),
            "max_ns":  arr.max(),
            "std_ns":  arr.std(),
        })

    rows.sort(key=lambda r: r["mean_ns"])

    col_w = max(len(r["file"]) for r in rows) + 2
    hdr = (
        f"{'Rank':<5} {'File':<{col_w}} {'N':>5} "
        f"{'Mean(ns)':>12} {'Min(ns)':>10} {'Max(ns)':>10} {'Std(ns)':>10}"
    )
    sep = "-" * len(hdr)
    print("\n" + sep)
    print(hdr)
    print(sep)
    for rank, r in enumerate(rows, 1):
        print(
            f"{rank:<5} {r['file']:<{col_w}} {r['n']:>5} "
            f"{r['mean_ns']:>12.1f} {r['min_ns']:>10.0f} "
            f"{r['max_ns']:>10.0f} {r['std_ns']:>10.1f}"
        )
    print(sep + "\n")

# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python plot_delay_errors.py file1.txt [file2.txt ...]")
        sys.exit(1)

    datasets: dict[str, list[int]] = {}
    for path in sys.argv[1:]:
        label = os.path.basename(path)
        try:
            datasets[label] = parse_file(path)
            print(f"Loaded '{label}': {len(datasets[label])} samples")
        except (OSError, ValueError) as exc:
            print(f"WARNING: {exc}", file=sys.stderr)

    if not datasets:
        print("No valid data files. Exiting.", file=sys.stderr)
        sys.exit(1)

    print_table(datasets)

    out_path = "delay_errors.png"
    plot(datasets, out_path)


if __name__ == "__main__":
    main()
