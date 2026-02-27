#!/usr/bin/env python3
"""
Generate adjacency graph data from mandala polygon data.

Supports:
- JSON polygon input (preferred): output from svg_to_polygons.js
- C++ region declarations input: src/database/*/*_regions.cpp (legacy)

Adjacency detection is tolerant to slight boundary gaps by using near-collinear,
near-overlapping segment matching with configurable thresholds.
"""

from __future__ import annotations

import argparse
import json
import math
import re
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


Point = Tuple[float, float]
RegionPolygons = Dict[int, List[Point]]
Pair = Tuple[int, int]


@dataclass
class Edge:
    region_id: int
    a: Point
    b: Point
    min_x: float
    min_y: float
    max_x: float
    max_y: float
    length: float


@dataclass
class DetectionConfig:
    eps_vertex: float
    eps_edge: float
    min_overlap: float
    min_shared_len: float
    min_contacts: int
    cell_size: float


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


def dist(a: Point, b: Point) -> float:
    return math.hypot(a[0] - b[0], a[1] - b[1])


def dot(a: Point, b: Point) -> float:
    return a[0] * b[0] + a[1] * b[1]


def sub(a: Point, b: Point) -> Point:
    return (a[0] - b[0], a[1] - b[1])


def interval_overlap(a0: float, a1: float, b0: float, b1: float) -> float:
    lo = max(min(a0, a1), min(b0, b1))
    hi = min(max(a0, a1), max(b0, b1))
    return max(0.0, hi - lo)


def distance_point_to_segment(p: Point, a: Point, b: Point) -> float:
    ab = sub(b, a)
    ap = sub(p, a)
    denom = dot(ab, ab)
    if denom <= 0.0:
        return dist(p, a)
    t = clamp(dot(ap, ab) / denom, 0.0, 1.0)
    closest = (a[0] + t * ab[0], a[1] + t * ab[1])
    return dist(p, closest)


def segment_distance(a: Point, b: Point, c: Point, d: Point) -> float:
    return min(
        distance_point_to_segment(a, c, d),
        distance_point_to_segment(b, c, d),
        distance_point_to_segment(c, a, b),
        distance_point_to_segment(d, a, b),
    )


def clean_polygon(points: Sequence[Point], eps_vertex: float) -> List[Point]:
    cleaned: List[Point] = []
    for p in points:
        if not cleaned or dist(cleaned[-1], p) > eps_vertex:
            cleaned.append((float(p[0]), float(p[1])))

    if len(cleaned) >= 2 and dist(cleaned[0], cleaned[-1]) <= eps_vertex:
        cleaned.pop()

    if len(cleaned) < 3:
        return []

    simplified: List[Point] = []
    for i in range(len(cleaned)):
        prev_p = cleaned[(i - 1) % len(cleaned)]
        cur_p = cleaned[i]
        next_p = cleaned[(i + 1) % len(cleaned)]

        if dist(prev_p, cur_p) <= eps_vertex and dist(cur_p, next_p) <= eps_vertex:
            continue

        simplified.append(cur_p)

    return simplified if len(simplified) >= 3 else []


def parse_json_polygons(json_path: Path) -> RegionPolygons:
    with json_path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    if isinstance(data, dict) and "regions" in data:
        data = data["regions"]

    regions: RegionPolygons = {}
    for idx, poly in enumerate(data):
        points = poly.get("points", [])
        if len(points) >= 3:
            regions[idx] = [(float(p[0]), float(p[1])) for p in points]
    return regions


def parse_cpp_regions(cpp_path: Path) -> RegionPolygons:
    text = cpp_path.read_text(encoding="utf-8")

    region_start = re.compile(r"regions\.emplace_back\((\d+),\s*std::vector<Vector2>\{", re.MULTILINE)
    vertex_re = re.compile(
        r"\{\s*offsetX\s*\+\s*([-+]?\d*\.?\d+)f\s*,\s*offsetY\s*\+\s*([-+]?\d*\.?\d+)f\s*\}"
    )

    regions: RegionPolygons = {}
    starts = list(region_start.finditer(text))
    for i, m in enumerate(starts):
        region_id = int(m.group(1))
        start_idx = m.end()
        end_idx = starts[i + 1].start() if i + 1 < len(starts) else len(text)
        block = text[start_idx:end_idx]

        block_points: List[Point] = []
        for vm in vertex_re.finditer(block):
            x = float(vm.group(1))
            y = float(vm.group(2))
            block_points.append((x, y))

        if len(block_points) >= 3:
            regions[region_id] = block_points

    return regions


def parse_region_id_list(raw_value: str) -> List[int]:
    if not raw_value.strip():
        return []

    parsed_ids: List[int] = []
    for token in raw_value.split(","):
        token = token.strip()
        if not token:
            continue
        parsed_ids.append(int(token))

    return sorted(set(parsed_ids))


def exclude_regions(regions: RegionPolygons, excluded_ids: Sequence[int]) -> RegionPolygons:
    if not excluded_ids:
        return regions

    excluded_set = set(excluded_ids)
    return {region_id: points for region_id, points in regions.items() if region_id not in excluded_set}


def compute_bounds(regions: RegionPolygons) -> Tuple[float, float, float, float]:
    xs = [p[0] for pts in regions.values() for p in pts]
    ys = [p[1] for pts in regions.values() for p in pts]
    return min(xs), min(ys), max(xs), max(ys)


def build_edges(regions: RegionPolygons, eps_vertex: float) -> List[Edge]:
    edges: List[Edge] = []
    for region_id in sorted(regions.keys()):
        polygon = clean_polygon(regions[region_id], eps_vertex)
        if len(polygon) < 3:
            continue

        n = len(polygon)
        for i in range(n):
            a = polygon[i]
            b = polygon[(i + 1) % n]
            length = dist(a, b)
            if length <= eps_vertex:
                continue

            edges.append(
                Edge(
                    region_id=region_id,
                    a=a,
                    b=b,
                    min_x=min(a[0], b[0]),
                    min_y=min(a[1], b[1]),
                    max_x=max(a[0], b[0]),
                    max_y=max(a[1], b[1]),
                    length=length,
                )
            )

    return edges


def edge_shared_length_like(e1: Edge, e2: Edge, cfg: DetectionConfig) -> float:
    if segment_distance(e1.a, e1.b, e2.a, e2.b) > cfg.eps_edge:
        return 0.0

    u = sub(e1.b, e1.a)
    v = sub(e2.b, e2.a)
    lu = math.hypot(u[0], u[1])
    lv = math.hypot(v[0], v[1])
    if lu <= 0.0 or lv <= 0.0:
        return 0.0

    # Require near-parallel segments.
    cross_norm = abs(u[0] * v[1] - u[1] * v[0]) / (lu * lv)
    if cross_norm > 0.25:  # ~14 degrees
        return 0.0

    dir_vec = (u[0] / lu, u[1] / lu) if lu >= lv else (v[0] / lv, v[1] / lv)
    normal = (-dir_vec[1], dir_vec[0])

    t1a = dot(e1.a, dir_vec)
    t1b = dot(e1.b, dir_vec)
    t2a = dot(e2.a, dir_vec)
    t2b = dot(e2.b, dir_vec)

    overlap = interval_overlap(t1a, t1b, t2a, t2b)
    if overlap < cfg.min_overlap:
        return 0.0

    n1 = (dot(e1.a, normal) + dot(e1.b, normal)) * 0.5
    n2 = (dot(e2.a, normal) + dot(e2.b, normal)) * 0.5
    lateral_sep = abs(n1 - n2)
    if lateral_sep > cfg.eps_edge * 1.35:
        return 0.0

    return overlap


def get_grid_cells(edge: Edge, cell_size: float, margin: float) -> Iterable[Tuple[int, int]]:
    min_cx = int(math.floor((edge.min_x - margin) / cell_size))
    max_cx = int(math.floor((edge.max_x + margin) / cell_size))
    min_cy = int(math.floor((edge.min_y - margin) / cell_size))
    max_cy = int(math.floor((edge.max_y + margin) / cell_size))

    for cx in range(min_cx, max_cx + 1):
        for cy in range(min_cy, max_cy + 1):
            yield (cx, cy)


def detect_adjacency_pairs(regions: RegionPolygons, cfg: DetectionConfig) -> List[Pair]:
    edges = build_edges(regions, cfg.eps_vertex)
    if not edges:
        return []

    grid: Dict[Tuple[int, int], List[int]] = defaultdict(list)
    for idx, edge in enumerate(edges):
        for cell in get_grid_cells(edge, cfg.cell_size, cfg.eps_edge):
            grid[cell].append(idx)

    tested_edge_pairs: set[Tuple[int, int]] = set()
    shared_length_by_pair: Dict[Pair, float] = defaultdict(float)
    contact_count_by_pair: Dict[Pair, int] = defaultdict(int)

    for edge_indices in grid.values():
        count = len(edge_indices)
        if count < 2:
            continue

        for i in range(count - 1):
            idx_a = edge_indices[i]
            edge_a = edges[idx_a]
            for j in range(i + 1, count):
                idx_b = edge_indices[j]
                if idx_a == idx_b:
                    continue

                edge_b = edges[idx_b]
                if edge_a.region_id == edge_b.region_id:
                    continue

                edge_pair = (min(idx_a, idx_b), max(idx_a, idx_b))
                if edge_pair in tested_edge_pairs:
                    continue
                tested_edge_pairs.add(edge_pair)

                shared_like = edge_shared_length_like(edge_a, edge_b, cfg)
                if shared_like <= 0.0:
                    continue

                pair = (min(edge_a.region_id, edge_b.region_id), max(edge_a.region_id, edge_b.region_id))
                shared_length_by_pair[pair] += shared_like
                contact_count_by_pair[pair] += 1

    accepted_pairs: List[Pair] = []
    for pair in sorted(shared_length_by_pair.keys()):
        shared_len = shared_length_by_pair[pair]
        contacts = contact_count_by_pair[pair]
        if shared_len >= cfg.min_shared_len or (contacts >= cfg.min_contacts and shared_len >= cfg.min_overlap):
            accepted_pairs.append(pair)

    return accepted_pairs


def default_detection_config(regions: RegionPolygons, args: argparse.Namespace) -> DetectionConfig:
    min_x, min_y, max_x, max_y = compute_bounds(regions)
    diagonal = math.hypot(max_x - min_x, max_y - min_y)

    eps_vertex = args.eps_vertex if args.eps_vertex is not None else clamp(diagonal * 0.00008, 0.5, 1.5)
    eps_edge = args.eps_edge if args.eps_edge is not None else clamp(diagonal * 0.004, 10.0, 120.0)
    min_overlap = args.min_overlap if args.min_overlap is not None else max(2.0, eps_edge * 0.06)
    min_shared_len = args.min_shared_len if args.min_shared_len is not None else max(10.0, eps_edge * 0.3)
    cell_size = args.cell_size if args.cell_size is not None else max(min_shared_len * 2.0, eps_edge * 4.0)

    return DetectionConfig(
        eps_vertex=eps_vertex,
        eps_edge=eps_edge,
        min_overlap=min_overlap,
        min_shared_len=min_shared_len,
        min_contacts=args.min_contacts,
        cell_size=cell_size,
    )


def generate_cpp(adjacency_pairs: Sequence[Pair], function_name: str) -> str:
    lines: List[str] = []
    lines.append('#include "../mandalaDatabase.h"')
    lines.append("")
    lines.append(f"void MandalaDatabase::{function_name}(AdjacencyGraph& adjacencyGraph) {{")
    for a, b in adjacency_pairs:
        lines.append(f"    adjacencyGraph.addAdjacency({a}, {b});")
    lines.append("}")
    return "\n".join(lines)


def generate_json(adjacency_pairs: Sequence[Pair], mandala_id: int | None, cfg: DetectionConfig) -> str:
    payload = {
        "mandala_id": mandala_id,
        "pair_count": len(adjacency_pairs),
        "pairs": [[a, b] for a, b in adjacency_pairs],
        "generation": {
            "eps_vertex": cfg.eps_vertex,
            "eps_edge": cfg.eps_edge,
            "min_overlap": cfg.min_overlap,
            "min_shared_len": cfg.min_shared_len,
            "min_contacts": cfg.min_contacts,
            "cell_size": cfg.cell_size,
        },
    }
    return json.dumps(payload, indent=2)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate adjacency graph JSON (or optional C++) from polygon data."
    )
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--json", type=Path, help="Input polygon JSON file")
    source.add_argument("--regions-cpp", type=Path, help="Input *_regions.cpp file")

    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("../resources/assets/3/mandala_3_adjacency.json"),
        help="Output file path (default: ../resources/assets/3/mandala_3_adjacency.json)",
    )
    parser.add_argument(
        "--format",
        choices=["json", "cpp"],
        default="json",
        help="Output format: json (default) or cpp",
    )
    parser.add_argument(
        "--function-name",
        default="addRealMandalaAdjacency",
        help="MandalaDatabase method name for --format cpp",
    )
    parser.add_argument("--mandala-id", type=int, default=None, help="Mandala id metadata for JSON output")
    parser.add_argument("--eps-vertex", type=float, default=None, help="Vertex dedupe tolerance")
    parser.add_argument("--eps-edge", type=float, default=None, help="Edge gap tolerance")
    parser.add_argument("--min-overlap", type=float, default=None, help="Minimum projected overlap")
    parser.add_argument("--min-shared-len", type=float, default=None, help="Minimum shared-like boundary length")
    parser.add_argument("--min-contacts", type=int, default=2, help="Minimum edge contact count for fallback acceptance")
    parser.add_argument("--cell-size", type=float, default=None, help="Spatial grid cell size")
    parser.add_argument(
        "--exclude-regions",
        type=str,
        default="",
        help="Comma-separated region IDs to exclude from adjacency generation (example: 0,14,27)",
    )
    parser.add_argument("--stdout", action="store_true", help="Print generated C++ to stdout")

    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if args.json is not None:
        regions = parse_json_polygons(args.json)
        input_label = str(args.json)
    else:
        regions = parse_cpp_regions(args.regions_cpp)
        input_label = str(args.regions_cpp)

    if not regions:
        raise ValueError("No valid regions found in input.")

    excluded_ids = parse_region_id_list(args.exclude_regions)
    regions = exclude_regions(regions, excluded_ids)

    if not regions:
        raise ValueError("No regions left after applying exclusions.")

    cfg = default_detection_config(regions, args)
    pairs = detect_adjacency_pairs(regions, cfg)
    if args.format == "cpp":
        output_data = generate_cpp(pairs, args.function_name)
    else:
        output_data = generate_json(pairs, args.mandala_id, cfg)

    print(f"Input: {input_label}")
    print(f"Regions: {len(regions)}")
    if excluded_ids:
        print(f"Excluded regions: {excluded_ids}")
    print(f"Adjacency pairs: {len(pairs)}")
    print(
        "Tolerances: "
        f"eps_vertex={cfg.eps_vertex:.4f}, "
        f"eps_edge={cfg.eps_edge:.4f}, "
        f"min_overlap={cfg.min_overlap:.4f}, "
        f"min_shared_len={cfg.min_shared_len:.4f}, "
        f"min_contacts={cfg.min_contacts}, "
        f"cell_size={cfg.cell_size:.4f}"
    )

    if args.stdout:
        print("\n" + output_data)
    else:
        output_path = args.output
        if not output_path.is_absolute():
            output_path = (Path(__file__).resolve().parent / output_path).resolve()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(output_data + "\n", encoding="utf-8")
        print(f"Wrote: {output_path}")


if __name__ == "__main__":
    main()
