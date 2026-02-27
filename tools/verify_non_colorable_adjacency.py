#!/usr/bin/env python3
"""
Verify that non-colorable regions are absent from adjacency pairs.

By default, validates every mandala entry in resources/assets/mandalas_manifest.json.
You can also scope validation to one mandala id with --mandala-id.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple


Pair = Tuple[int, int]


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def parse_pairs(adjacency_data: Dict) -> List[Pair]:
    raw_pairs = adjacency_data.get("pairs", [])
    pairs: List[Pair] = []

    for item in raw_pairs:
        if not isinstance(item, list) or len(item) < 2:
            continue
        a = int(item[0])
        b = int(item[1])
        if a == b:
            continue
        lo, hi = (a, b) if a < b else (b, a)
        pairs.append((lo, hi))

    return pairs


def non_colorable_region_ids(regions_data: Dict) -> Set[int]:
    result: Set[int] = set()
    for region in regions_data.get("regions", []):
        if not isinstance(region, dict):
            continue
        region_id = region.get("id")
        if region_id is None:
            continue
        is_colorable = bool(region.get("colorable", True))
        if not is_colorable:
            result.add(int(region_id))
    return result


def violating_pairs(non_colorable: Set[int], pairs: Iterable[Pair]) -> List[Pair]:
    violations: List[Pair] = []
    for a, b in pairs:
        if a in non_colorable or b in non_colorable:
            violations.append((a, b))
    return violations


def resolve_asset_path(root: Path, relative_or_absolute: str) -> Path:
    candidate = Path(relative_or_absolute)
    if candidate.is_absolute():
        return candidate

    if candidate.exists():
        return candidate

    return root / "resources" / "assets" / candidate


def verify_manifest_entry(root: Path, entry: Dict) -> Tuple[bool, str]:
    mandala_id = int(entry["id"])
    name = str(entry.get("name", f"Mandala {mandala_id}"))

    regions_path = resolve_asset_path(root, str(entry["regions"]))
    adjacency_path = resolve_asset_path(root, str(entry["adjacency"]))

    if not regions_path.exists():
        return False, f"[FAIL] id={mandala_id} ({name}) missing regions file: {regions_path}"
    if not adjacency_path.exists():
        return False, f"[FAIL] id={mandala_id} ({name}) missing adjacency file: {adjacency_path}"

    regions_data = load_json(regions_path)
    adjacency_data = load_json(adjacency_path)

    non_colorable = non_colorable_region_ids(regions_data)
    pairs = parse_pairs(adjacency_data)
    violations = violating_pairs(non_colorable, pairs)

    if violations:
        preview = ", ".join(f"[{a},{b}]" for a, b in violations[:10])
        extra = "" if len(violations) <= 10 else f" ... (+{len(violations) - 10} more)"
        return (
            False,
            f"[FAIL] id={mandala_id} ({name}) found {len(violations)} invalid adjacency pair(s): {preview}{extra}",
        )

    return True, f"[OK] id={mandala_id} ({name}) non-colorable regions are absent from adjacency graph"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify that non-colorable regions are not referenced in adjacency pairs."
    )
    parser.add_argument(
        "--mandala-id",
        type=int,
        default=None,
        help="Optional manifest mandala id to verify; defaults to all entries.",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("resources/assets/mandalas_manifest.json"),
        help="Path to manifest JSON (default: resources/assets/mandalas_manifest.json)",
    )
    args = parser.parse_args()

    root = repo_root()
    manifest_path = args.manifest
    if not manifest_path.is_absolute():
        manifest_path = root / manifest_path

    if not manifest_path.exists():
        print(f"[ERROR] Manifest file not found: {manifest_path}", file=sys.stderr)
        return 2

    manifest_data = load_json(manifest_path)
    if not isinstance(manifest_data, list):
        print("[ERROR] Manifest root must be a JSON array", file=sys.stderr)
        return 2

    entries: List[Dict] = []
    for item in manifest_data:
        if isinstance(item, dict) and "id" in item and "regions" in item and "adjacency" in item:
            entries.append(item)

    if args.mandala_id is not None:
        entries = [entry for entry in entries if int(entry["id"]) == args.mandala_id]
        if not entries:
            print(f"[ERROR] No manifest entry found for --mandala-id={args.mandala_id}", file=sys.stderr)
            return 2

    all_ok = True
    for entry in entries:
        ok, message = verify_manifest_entry(root, entry)
        print(message)
        all_ok = all_ok and ok

    return 0 if all_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
