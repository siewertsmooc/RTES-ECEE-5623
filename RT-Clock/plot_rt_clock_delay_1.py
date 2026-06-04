#!/usr/bin/env python3
"""
plot_delay_errors.py — For each clock type under output/, load all trial
files, average each test index (0-99) across trials, then produce an
overlapping dot plot + ranked average error table.

Directory layout (hardcoded):
    output/
        clock_monotonic/   clock_monotonic_0.txt … clock_monotonic_N.txt
        clock_realtime/    clock_realtime_0.txt  … clock_realtime_N.txt
        ...

Each file contains 100 lines matching:
    MY_CLOCK delay error = 0, nanoseconds = <value>

AI USAGE DISCLAIMER:
    I do not take any credit for the code developed herein.
    I generated this code with Claude AI to help me udnerstand the RT Clock code.
    Find the thread here: https://claude.ai/share/e0b5a8cc-74e5-4e66-8f48-80ccdc1fe46d
    Use code with caution.

"""

import os
import re
import glob
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np

# ── Hardcoded paths ───────────────────────────────────────────────────────────

SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
OUTPUT_ROOT  = os.path.join(SCRIPT_DIR, "output")
PLOT_PATH    = os.path.join(SCRIPT_DIR, "delay_errors.png")
NUM_TESTS    = 100   # expected samples per trial file

# ── Parser ────────────────────────────────────────────────────────────────────

LINE_RE = re.compile(
    r"MY_CLOCK delay error\s*=\s*\d+,\s*nanoseconds\s*=\s*(\d+)"
)

def parse_file(path: str) -> list[int]:
    """Return list of delay error ns values in file order."""
    errors = []
    with open(path, "r") as fh:
        for line in fh:
            m = LINE_RE.search(line)
            if m:
                errors.append(int(m.group(1)))
    if not errors:
        raise ValueError(f"No delay error lines found in '{path}'")
    return errors

# ── Loader ────────────────────────────────────────────────────────────────────

def load_clock_series(clock_dir: str) -> np.ndarray:
    """
    Load all trial files from clock_dir, return array shaped
    (num_trials, NUM_TESTS).  Trials with wrong sample count are skipped.
    """
    pattern = os.path.join(clock_dir, "*.txt")
    paths   = sorted(glob.glob(pattern))
    if not paths:
        raise ValueError(f"No .txt files found in '{clock_dir}'")

    trials = []
    for p in paths:
        try:
            samples = parse_file(p)
        except ValueError as exc:
            print(f"  WARNING: {exc} — skipping")
            continue
        if len(samples) != NUM_TESTS:
            print(f"  WARNING: '{os.path.basename(p)}' has {len(samples)} samples "
                  f"(expected {NUM_TESTS}) — skipping")
            continue
        trials.append(samples)

    if not trials:
        raise ValueError(f"No valid trials loaded from '{clock_dir}'")

    return np.array(trials, dtype=float)   # shape: (trials, NUM_TESTS)

def discover_clocks(root: str) -> dict[str, np.ndarray]:
    """
    Walk root/, treat each subdirectory as a clock type.
    Returns {clock_name: array(trials, NUM_TESTS)}.
    """
    clocks = {}
    for entry in sorted(os.scandir(root), key=lambda e: e.name):
        if not entry.is_dir():
            continue
        print(f"Loading '{entry.name}' ...")
        try:
            clocks[entry.name] = load_clock_series(entry.path)
            t, s = clocks[entry.name].shape
            print(f"  → {t} trial(s), {s} tests each")
        except ValueError as exc:
            print(f"  ERROR: {exc} — skipping clock")
    return clocks

# ── Stats table ───────────────────────────────────────────────────────────────

def print_table(clocks: dict[str, np.ndarray]) -> None:
    """Print per-clock summary ranked by overall mean delay error."""
    rows = []
    for name, data in clocks.items():
        # data: (trials, NUM_TESTS)
        per_test_mean = data.mean(axis=0)          # shape: (NUM_TESTS,)
        rows.append({
            "name":     name,
            "trials":   data.shape[0],
            "mean_ns":  per_test_mean.mean(),
            "min_ns":   per_test_mean.min(),
            "max_ns":   per_test_mean.max(),
            "std_ns":   per_test_mean.std(),
        })

    rows.sort(key=lambda r: r["mean_ns"])

    col_w = max(len(r["name"]) for r in rows) + 2
    hdr = (
        f"{'Rank':<5} {'Clock':<{col_w}} {'Trials':>7} "
        f"{'Mean(ns)':>12} {'Min(ns)':>10} {'Max(ns)':>10} {'Std(ns)':>10}"
    )
    sep = "-" * len(hdr)
    print("\n" + sep)
    print(hdr)
    print(sep)
    for rank, r in enumerate(rows, 1):
        print(
            f"{rank:<5} {r['name']:<{col_w}} {r['trials']:>7} "
            f"{r['mean_ns']:>12.1f} {r['min_ns']:>10.1f} "
            f"{r['max_ns']:>10.1f} {r['std_ns']:>10.1f}"
        )
    print(sep + "\n")

# ── Plot ──────────────────────────────────────────────────────────────────────

def plot(clocks: dict[str, np.ndarray], out_path: str) -> None:
    fig, ax = plt.subplots(figsize=(14, 6))

    colors  = cm.tab10(np.linspace(0, 1, max(len(clocks), 1)))
    indices = np.arange(NUM_TESTS)

    for (name, data), color in zip(clocks.items(), colors):
        per_test_mean = data.mean(axis=0)   # average across trials per test
        ax.scatter(
            indices,
            per_test_mean,
            label=name,
            color=color,
            s=20,
            alpha=0.80,
            linewidths=0,
        )

    ax.set_xlabel("Test index", fontsize=12)
    ax.set_ylabel("Mean delay error (ns)", fontsize=12)
    ax.set_title(
        f"MY_CLOCK mean delay error per test index\n"
        f"(averaged across trials)",
        fontsize=13,
    )
    ax.legend(title="Clock", bbox_to_anchor=(1.01, 1), loc="upper left", fontsize=9)
    ax.grid(axis="y", linestyle="--", alpha=0.4)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"Plot saved → {out_path}")

# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    if not os.path.isdir(OUTPUT_ROOT):
        print(f"ERROR: output directory not found: '{OUTPUT_ROOT}'")
        raise SystemExit(1)

    clocks = discover_clocks(OUTPUT_ROOT)
    if not clocks:
        print("ERROR: no clock data loaded.")
        raise SystemExit(1)

    print_table(clocks)
    plot(clocks, PLOT_PATH)


if __name__ == "__main__":
    main()
