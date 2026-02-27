#!/usr/bin/env python3
"""
JSON to SVG Converter

Converts JSON polygon files (from svg_to_polygons.js) back to SVG format
with <polygon> elements.

Usage:
    python polygons_to_svg.py input.json [output.svg]

Arguments:
    input.json      Input JSON file with polygon data
    output.svg      Output SVG file (default: input_polygons.svg)
"""

import json
import sys
import os
from pathlib import Path


class Polygon:
    def __init__(self, points, closed=True):
        self.points = points
        self.closed = closed

    def to_svg(self):
        svg = "<polygon points=\""
        for point in self.points:
            svg += f"{point[0]},{point[1]} "
        svg += "\" style=\"fill:none;stroke:black;stroke-width:1\" />"
        return svg


def compute_bounds(polygons):
    all_x = []
    all_y = []
    for polygon in polygons:
        for point in polygon.points:
            all_x.append(float(point[0]))
            all_y.append(float(point[1]))

    if not all_x or not all_y:
        return 0.0, 0.0, 800.0, 600.0

    min_x = min(all_x)
    max_x = max(all_x)
    min_y = min(all_y)
    max_y = max(all_y)

    width = max(1.0, max_x - min_x)
    height = max(1.0, max_y - min_y)
    return min_x, min_y, width, height


def main():
    # Parse command line arguments
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    # Generate output filename if not provided
    if len(sys.argv) >= 3:
        output_file = sys.argv[2]
    else:
        base = Path(input_file).stem
        output_file = f"{base}_polygons.svg"
    
    # Check input file exists
    if not os.path.exists(input_file):
        print(f"Error: Input file not found: {input_file}")
        sys.exit(1)
    
    print(f"Reading JSON from: {input_file}")
    
    # Read JSON
    with open(input_file) as f:
        data = json.load(f)
    
    # Support both raw polygon arrays and runtime regions JSON objects
    if isinstance(data, dict):
        data = data.get("regions", [])

    # Create polygons
    polygons = []
    for polygon_data in data:
        if not isinstance(polygon_data, dict):
            continue
        points = polygon_data.get("points", [])
        if len(points) < 3:
            continue
        polygons.append(Polygon(points, polygon_data.get("closed", True)))
    
    print(f"Found {len(polygons)} polygons")
    
    min_x, min_y, width, height = compute_bounds(polygons)

    # Generate SVG
    svg_lines = [
        f"<?xml version=\"1.0\" encoding=\"UTF-8\"?><svg id=\"a\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"{min_x} {min_y} {width} {height}\">\n"
    ]
    
    for polygon in polygons:
        svg_lines.append(polygon.to_svg() + "\n")
    
    svg_lines.append("</svg>")
    
    # Write output
    with open(output_file, 'w') as f:
        f.writelines(svg_lines)
    
    print(f"Wrote SVG to: {output_file}")


if __name__ == '__main__':
    main()
