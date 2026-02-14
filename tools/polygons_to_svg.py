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
        svg += "\" style=\"fill:white;stroke:black;stroke-width:1\" />"
        return svg


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
    
    # Create polygons
    polygons = []
    for polygon_data in data:
        polygons.append(Polygon(polygon_data["points"], polygon_data.get("closed", True)))
    
    print(f"Found {len(polygons)} polygons")
    
    # Generate SVG
    svg_lines = [
        """<?xml version="1.0" encoding="UTF-8"?><svg id="a" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 800 600.7">\n"""
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
