#!/usr/bin/env python3
"""
JSON to Mandala Regions JSON generator.

Reads polygon JSON files produced by svg_to_polygons.js and generates a
runtime-ready regions JSON file for the app.

Usage:
    python json_to_mandala_code.py ../resources/assets/3/mandala_1.json \
      --name "My Mandala" --id 3 \
      --output ../resources/assets/3/mandala_3_regions.json
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import List, Tuple


class Point:
    def __init__(self, x: float, y: float):
        self.x = round(float(x), 3)
        self.y = round(float(y), 3)


class Polygon:
    def __init__(self, index: int, points: List[List[float]], closed: bool):
        self.index = index
        self.points = self._clean_points(points)
        self.closed = closed
        self.is_clockwise = self._calculate_winding()

    @staticmethod
    def _almost_equal(a: Point, b: Point, epsilon: float = 1e-4) -> bool:
        return abs(a.x - b.x) <= epsilon and abs(a.y - b.y) <= epsilon

    def _clean_points(self, raw_points: List[List[float]]) -> List[Point]:
        cleaned: List[Point] = []
        for raw_point in raw_points:
            if not isinstance(raw_point, list) or len(raw_point) < 2:
                continue
            point = Point(raw_point[0], raw_point[1])
            if not cleaned or not self._almost_equal(cleaned[-1], point):
                cleaned.append(point)

        if len(cleaned) >= 2 and self._almost_equal(cleaned[0], cleaned[-1]):
            cleaned.pop()

        return cleaned

    def _calculate_winding(self) -> bool:
        if len(self.points) < 3:
            return True

        area = 0.0
        for index in range(len(self.points)):
            next_index = (index + 1) % len(self.points)
            area += self.points[index].x * self.points[next_index].y
            area -= self.points[next_index].x * self.points[index].y

        return area < 0

    def reverse_if_needed(self, target_clockwise: bool) -> None:
        if self.is_clockwise != target_clockwise:
            self.points.reverse()
            self.is_clockwise = target_clockwise


def parse_json_polygons(json_file: Path) -> List[Polygon]:
    with json_file.open("r", encoding="utf-8") as file:
        data = json.load(file)

    polygons: List[Polygon] = []
    for index, polygon_data in enumerate(data):
        points = polygon_data.get("points", [])
        closed = bool(polygon_data.get("closed", True))

        polygon = Polygon(index, points, closed)
        if len(polygon.points) >= 3:
            polygons.append(polygon)

    return polygons


def calculate_bounds(polygons: List[Polygon]) -> Tuple[Point, Point]:
    all_x = [point.x for polygon in polygons for point in polygon.points]
    all_y = [point.y for polygon in polygons for point in polygon.points]

    return Point(min(all_x), min(all_y)), Point(max(all_x), max(all_y))


def scale_polygons_to_target_width(polygons: List[Polygon], target_width: float) -> float:
    if target_width <= 0:
        return 1.0

    min_bound, max_bound = calculate_bounds(polygons)
    current_width = max_bound.x - min_bound.x
    if current_width <= 0.0:
        return 1.0

    scale = target_width / current_width
    if abs(scale - 1.0) <= 1e-6:
        return 1.0

    center_x = (min_bound.x + max_bound.x) * 0.5
    center_y = (min_bound.y + max_bound.y) * 0.5

    for polygon in polygons:
        for point in polygon.points:
            point.x = round((point.x - center_x) * scale + center_x, 3)
            point.y = round((point.y - center_y) * scale + center_y, 3)

    return scale


def to_regions_payload(
    polygons: List[Polygon],
    mandala_name: str,
    mandala_id: int,
    normalize_winding: bool,
    target_clockwise: bool,
    target_width: float,
) -> dict:
    if normalize_winding:
        for polygon in polygons:
            polygon.reverse_if_needed(target_clockwise)

    applied_scale = scale_polygons_to_target_width(polygons, target_width)

    min_bound, max_bound = calculate_bounds(polygons)
    center_x = round((min_bound.x + max_bound.x) / 2.0, 3)
    center_y = round((min_bound.y + max_bound.y) / 2.0, 3)
    width = round(max_bound.x - min_bound.x, 3)

    regions = []
    for polygon in polygons:
        regions.append(
            {
                "id": polygon.index,
                "closed": polygon.closed,
                "points": [[point.x, point.y] for point in polygon.points],
            }
        )

    return {
        "id": mandala_id,
        "name": mandala_name,
        "source_center": [center_x, center_y],
        "source_width": width,
        "source_scale": round(applied_scale, 6),
        "region_count": len(regions),
        "regions": regions,
    }


def infer_output_path(input_json: Path, mandala_id: int) -> Path:
    parent = input_json.parent
    return parent / f"mandala_{mandala_id}_regions.json"


def generate_summary(polygons: List[Polygon], winding_normalized: bool) -> str:
    clockwise_count = sum(1 for polygon in polygons if polygon.is_clockwise)
    counter_clockwise_count = len(polygons) - clockwise_count

    lines = [
        "=" * 60,
        "REGIONS JSON SUMMARY",
        "=" * 60,
        f"Total Regions: {len(polygons)}",
        f"Winding: {counter_clockwise_count} counter-clockwise, {clockwise_count} clockwise",
    ]

    if winding_normalized:
        lines.append("Winding normalized for tessellation stability")

    lines.extend(
        [
            "",
            "Next step:",
            "  Generate adjacency JSON with generate_adjacency.py",
            "=" * 60,
        ]
    )

    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate runtime regions JSON from svg_to_polygons output"
    )
    parser.add_argument("json_file", type=Path, help="Input JSON from svg_to_polygons.js")
    parser.add_argument("-n", "--name", required=True, help="Mandala name")
    parser.add_argument("-i", "--id", type=int, required=True, help="Mandala id")
    parser.add_argument("-o", "--output", type=Path, help="Output regions JSON path")
    parser.add_argument("--no-summary", action="store_true", help="Disable summary output")
    parser.add_argument(
        "--no-normalize",
        action="store_true",
        help="Preserve original winding order (not recommended)",
    )
    parser.add_argument(
        "--counter-clockwise",
        action="store_true",
        help="Normalize to counter-clockwise winding (default: clockwise)",
    )
    parser.add_argument(
        "--target-width",
        type=float,
        default=10000.0,
        help="Scale polygons to this width in source units (default: 10000). Set <=0 to disable scaling.",
    )

    args = parser.parse_args()

    polygons = parse_json_polygons(args.json_file)
    if not polygons:
        raise RuntimeError(f"No polygons found in {args.json_file}")

    normalize_winding = not args.no_normalize
    target_clockwise = not args.counter_clockwise

    payload = to_regions_payload(
        polygons=polygons,
        mandala_name=args.name,
        mandala_id=args.id,
        normalize_winding=normalize_winding,
        target_clockwise=target_clockwise,
        target_width=args.target_width,
    )

    output_path = args.output if args.output else infer_output_path(args.json_file, args.id)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")

    print(f"Wrote regions JSON: {output_path}")
    if not args.no_summary:
        print(generate_summary(polygons, normalize_winding))


if __name__ == "__main__":
    main()
