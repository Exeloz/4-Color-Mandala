#!/usr/bin/env python3
"""
Generate a bank of distinct MiniZinc coloring solutions by retrying with random seeds.

The source model is expected to print lines like:
- min_colors=<int>
- colors=[c0, c1, ...]

This script keeps only unique color vectors and writes them to a text file in the
"colors=[...]" format consumed by SolutionFreezeDailyRuleset.
"""

from __future__ import annotations

import argparse
import random
import re
import subprocess
from pathlib import Path
from typing import List, Optional, Sequence, Tuple


MIN_COLORS_RE = re.compile(r"min_colors\s*=\s*(\d+)")
COLORS_RE = re.compile(r"colors\s*=\s*\[([^\]]+)\]")


def parse_solution_output(output: str) -> Tuple[Optional[int], Optional[Tuple[int, ...]]]:
    min_colors_match = MIN_COLORS_RE.search(output)
    colors_match = COLORS_RE.search(output)

    min_colors = int(min_colors_match.group(1)) if min_colors_match else None

    if not colors_match:
        return min_colors, None

    raw_values = colors_match.group(1).split(",")
    colors: List[int] = []
    for raw in raw_values:
        value = raw.strip()
        if not value:
            continue
        try:
            colors.append(int(value))
        except ValueError:
            return min_colors, None

    return min_colors, tuple(colors)


def run_minizinc(model_path: Path, solver: str, seed: int, minizinc_bin: str, timeout: int) -> Optional[str]:
    command = [
        minizinc_bin,
        "--solver",
        solver,
        "--random-seed",
        str(seed),
        str(model_path),
    ]

    try:
        completed = subprocess.run(
            command,
            check=False,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        # Timeout is treated as a failed attempt for this seed; caller will continue.
        return None

    if completed.returncode != 0:
        raise RuntimeError(
            "MiniZinc failed with seed "
            f"{seed}.\nSTDOUT:\n{completed.stdout}\nSTDERR:\n{completed.stderr}"
        )

    return completed.stdout


def build_output_path(model_path: Path, output_path: Optional[Path]) -> Path:
    if output_path is not None:
        return output_path

    stem = model_path.stem
    lower_name = stem.lower()
    if "hard" in lower_name:
        return model_path.parent / "daily_solutions_hard.txt"
    return model_path.parent / "daily_solutions.txt"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate N unique MiniZinc solutions by varying random seeds."
    )
    parser.add_argument("model", type=Path, help="Path to the .mzn model")
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output file (default: daily_solutions.txt or daily_solutions_hard.txt near model)",
    )
    parser.add_argument("--count", type=int, default=100, help="Target number of unique solutions")
    parser.add_argument("--solver", type=str, default="chuffed", help="MiniZinc solver name")
    parser.add_argument(
        "--seed",
        type=int,
        default=20260311,
        help="Seed for this script's PRNG (used to generate MiniZinc seeds)",
    )
    parser.add_argument(
        "--max-attempts",
        type=int,
        default=3000,
        help="Maximum MiniZinc runs before stopping",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=10,
        help="Per-run timeout in seconds (default: 10)",
    )
    parser.add_argument(
        "--minizinc-bin",
        type=str,
        default="minizinc",
        help="MiniZinc executable",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    model_path: Path = args.model.resolve()
    if not model_path.exists():
        raise FileNotFoundError(f"Model not found: {model_path}")

    if args.count <= 0:
        raise ValueError("--count must be > 0")

    if args.max_attempts <= 0:
        raise ValueError("--max-attempts must be > 0")

    output_path = build_output_path(model_path, args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    prng = random.Random(args.seed)

    unique_solutions: List[Tuple[int, ...]] = []
    seen = set()
    target_min_colors: Optional[int] = None

    attempts = 0
    while attempts < args.max_attempts and len(unique_solutions) < args.count:
        attempts += 1
        seed = prng.randint(1, 2_147_483_647)
        stdout = run_minizinc(model_path, args.solver, seed, args.minizinc_bin, args.timeout)
        if stdout is None:
            continue
        min_colors, solution = parse_solution_output(stdout)

        if solution is None:
            continue

        if target_min_colors is None and min_colors is not None:
            target_min_colors = min_colors

        if target_min_colors is not None and min_colors is not None and min_colors != target_min_colors:
            continue

        if solution in seen:
            continue

        seen.add(solution)
        unique_solutions.append(solution)

    if not unique_solutions:
        raise RuntimeError("No solution captured. Check solver/model output format.")

    with output_path.open("w", encoding="utf-8") as handle:
        handle.write(f"# Generated from {model_path.name}\n")
        if target_min_colors is not None:
            handle.write(f"# min_colors={target_min_colors}\n")
        handle.write(f"# unique_solutions={len(unique_solutions)} attempts={attempts}\n")
        for solution in unique_solutions:
            as_text = ",".join(str(value) for value in solution)
            handle.write(f"colors=[{as_text}]\n")

    print(
        f"Wrote {len(unique_solutions)} unique solution(s) to {output_path} "
        f"after {attempts} attempt(s)."
    )


if __name__ == "__main__":
    main()
