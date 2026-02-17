#!/usr/bin/env python3
"""
JSON to Mandala Code Generator

Reads JSON polygon files produced by svg_to_polygons.js and generates
C++ code for the mandala database. User handles adjacency graph manually.

Usage:
    python json_to_mandala_code.py test1.json --name "My Mandala" --id 3
"""

import json
import argparse
from typing import List, Tuple


class Point:
    def __init__(self, x: float, y: float):
        self.x = round(x, 2)
        self.y = round(y, 2)


class Polygon:
    def __init__(self, index: int, points: List[List[float]], closed: bool):
        self.index = index
        self.points = [Point(p[0], p[1]) for p in points]
        self.closed = closed
        self.is_clockwise = self._calculate_winding()
    
    def _calculate_winding(self) -> bool:
        """Calculate if polygon is clockwise using signed area.
        Returns True if clockwise, False if counter-clockwise."""
        if len(self.points) < 3:
            return True
        
        # Calculate signed area using shoelace formula
        area = 0.0
        for i in range(len(self.points)):
            j = (i + 1) % len(self.points)
            area += (self.points[i].x * self.points[j].y)
            area -= (self.points[j].x * self.points[i].y)
        
        # Negative area = clockwise, Positive = counter-clockwise
        return area < 0
    
    def reverse_if_needed(self, target_clockwise: bool):
        """Reverse vertex order if winding doesn't match target."""
        if self.is_clockwise != target_clockwise:
            self.points.reverse()
            self.is_clockwise = target_clockwise


def parse_json_polygons(json_file: str) -> List[Polygon]:
    """Parse JSON file with polygon data from svg_to_polygons.js"""
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    polygons = []
    for idx, poly_data in enumerate(data):
        points = poly_data.get('points', [])
        closed = poly_data.get('closed', True)
        
        if len(points) >= 3:  # Valid polygon needs at least 3 vertices
            polygons.append(Polygon(idx, points, closed))
    
    return polygons


def calculate_bounds(polygons: List[Polygon]) -> Tuple[Point, Point]:
    """Calculate bounding box of all polygons"""
    all_x = [p.x for poly in polygons for p in poly.points]
    all_y = [p.y for poly in polygons for p in poly.points]
    
    min_point = Point(min(all_x), min(all_y))
    max_point = Point(max(all_x), max(all_y))
    
    return min_point, max_point


def generate_cpp_code(polygons: List[Polygon], mandala_name: str, mandala_id: int, 
                      normalize_winding: bool = True, target_clockwise: bool = False) -> str:
    """Generate C++ code for the mandala database (without adjacency graph)
    
    Args:
        polygons: List of polygon objects
        mandala_name: Name of the mandala
        mandala_id: Unique ID for the mandala
        normalize_winding: If True, ensure all polygons have same winding order
        target_clockwise: Target winding order (False = counter-clockwise, True = clockwise)
    """
    
    # Normalize winding order if requested
    if normalize_winding:
        for poly in polygons:
            poly.reverse_if_needed(target_clockwise)
    
    # Calculate center offset for centering
    min_bound, max_bound = calculate_bounds(polygons)
    center_x = (min_bound.x + max_bound.x) / 2
    center_y = (min_bound.y + max_bound.y) / 2
    
    # Generate function name from mandala name
    func_name = ''.join(word.capitalize() for word in mandala_name.split())
    func_name = 'create' + func_name + 'Mandala'
    
    code_lines = []
    code_lines.append('#include "../mandalaDatabase.h"')
    code_lines.append('#include "../../ui/colors.h"')
    code_lines.append('')
    code_lines.append('    // regions.back().setDefaultColor(Colors::Black);')
    code_lines.append('    // regions.back().setColorable(false);')
    code_lines.append('namespace {')
    code_lines.append('    constexpr float SCREEN_CENTER_X = 400.0f;')
    code_lines.append('    constexpr float SCREEN_CENTER_Y = 300.0f;')
    code_lines.append('}')
    code_lines.append('')
    code_lines.append(f"void MandalaDatabase::{func_name}() {{")
    code_lines.append(f"    std::vector<Region> regions;")
    code_lines.append(f"    AdjacencyGraph adjacencyGraph({len(polygons)});")
    code_lines.append(f"    ")
    code_lines.append(f"    Vector2 center = {{SCREEN_CENTER_X, SCREEN_CENTER_Y}};")
    code_lines.append(f"    float offsetX = center.x - {center_x:.2f}f;")
    code_lines.append(f"    float offsetY = center.y - {center_y:.2f}f;")
    code_lines.append(f"")
    
    # Generate region creation code
    for poly in polygons:
        code_lines.append(f"    // Region {poly.index}")
        code_lines.append(f"    regions.emplace_back({poly.index}, std::vector<Vector2>{{")
        
        for i, point in enumerate(poly.points):
            comma = "," if i < len(poly.points) - 1 else ""
            code_lines.append(f"        {{offsetX + {point.x:.2f}f, offsetY + {point.y:.2f}f}}{comma}")
        
        code_lines.append(f"    }});")
        code_lines.append(f"")
    
    # Placeholder for adjacency graph
    code_lines.append(f"    // TODO: Add adjacency relationships manually")
    code_lines.append(f"    // You can keep these in a separate .cpp if you prefer.")
    code_lines.append(f"    // adjacencyGraph.addAdjacency(i, j);")
    code_lines.append(f"")
    code_lines.append(f"    auto mandala = std::make_shared<Mandala>({mandala_id}, \"{mandala_name}\", regions, adjacencyGraph);")
    code_lines.append(f"    mandalaList.push_back(mandala);")
    code_lines.append(f"}}")
    
    return '\n'.join(code_lines)


def generate_summary(polygons: List[Polygon], winding_normalized: bool = False) -> str:
    """Generate summary information about the polygons"""
    lines = []
    lines.append("\n" + "="*60)
    lines.append("POLYGON SUMMARY")
    lines.append("="*60)
    lines.append(f"Total Regions: {len(polygons)}")
    
    if winding_normalized:
        lines.append("✓ Winding order normalized for tesselation")
    
    lines.append("")
    
    # Count winding directions
    cw_count = sum(1 for p in polygons if p.is_clockwise)
    ccw_count = len(polygons) - cw_count
    lines.append(f"Winding: {ccw_count} counter-clockwise, {cw_count} clockwise")
    lines.append("")
    
    for poly in polygons:
        closed_str = "closed" if poly.closed else "open"
        winding_str = "CW" if poly.is_clockwise else "CCW"
        vertex_count = len(poly.points)
        lines.append(f"  Region {poly.index:3d}: {vertex_count} vertices ({closed_str}, {winding_str})")
    
    lines.append("")
    lines.append("Next steps:")
    lines.append("1. Review the generated regions")
    lines.append("2. Manually add adjacency relationships:")
    lines.append("   adjacencyGraph.addAdjacency(i, j);")
    lines.append("3. Test the mandala coloring to verify adjacencies")
    lines.append("="*60)
    
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(
        description='Generate C++ mandala database code from JSON polygons',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python json_to_mandala_code.py test1.json --name "Flower Pattern" --id 3
  python json_to_mandala_code.py mandala.json -n "Star" -i 4 -o mandala.cpp
  python json_to_mandala_code.py data.json -n "Test" -i 5 --clockwise
  
Input JSON format (from svg_to_polygons.js):
  [
    {
      "points": [[x1, y1], [x2, y2], ...],
      "closed": true
    },
    ...
  ]

Winding Order:
  By default, all polygons are normalized to counter-clockwise winding,
  which is standard for most tesselation libraries. Use --clockwise to
  reverse this, or --no-normalize to preserve original winding.
        """)
    
    parser.add_argument('json_file', help='Input JSON file from svg_to_polygons.js')
    parser.add_argument('-n', '--name', required=True, help='Name of the mandala')
    parser.add_argument('-i', '--id', type=int, required=True, help='Unique ID for the mandala')
    parser.add_argument('-o', '--output', help='Output file (default: print to stdout)')
    parser.add_argument('--no-summary', action='store_true', 
                       help='Skip polygon summary output')
    parser.add_argument('--no-normalize', action='store_true',
                       help='Do not normalize polygon winding order (may cause tesselation issues)')
    parser.add_argument('--clockwise', action='store_true',
                       help='Normalize to clockwise winding (default is counter-clockwise)')
    
    args = parser.parse_args()
    
    # Parse JSON
    print(f"Parsing {args.json_file}...", end=' ')
    polygons = parse_json_polygons(args.json_file)
    print(f"Found {len(polygons)} polygons")
    
    # Report winding before normalization
    cw_count = sum(1 for p in polygons if p.is_clockwise)
    ccw_count = len(polygons) - cw_count
    print(f"Original winding: {ccw_count} CCW, {cw_count} CW")
    
    # Generate code
    print("Generating C++ code...")
    normalize_winding = not args.no_normalize
    target_clockwise = args.clockwise
    
    if normalize_winding:
        target_str = "clockwise" if target_clockwise else "counter-clockwise"
        print(f"Normalizing all polygons to {target_str} winding...")
    
    cpp_code = generate_cpp_code(polygons, args.name, args.id, 
                                  normalize_winding, target_clockwise)
    
    # Output
    if args.output:
        with open(args.output, 'w') as f:
            f.write(cpp_code)
            if not args.no_summary:
                f.write('\n\n')
                f.write(generate_summary(polygons, normalize_winding))
        print(f"\nCode written to {args.output}")
    else:
        print("\n" + cpp_code)
    
    # Print summary
    if not args.no_summary:
        print(generate_summary(polygons, normalize_winding))
    
    # Print integration instructions
    print("\n" + "="*60)
    print("Don't forget to add this declaration to mandalaDatabase.h:")
    func_name = ''.join(word.capitalize() for word in args.name.split())
    print(f"    void create{func_name}Mandala();")
    print("\nAnd call it from createSampleMandala():")
    print(f"    create{func_name}Mandala();")
    print("="*60)


if __name__ == '__main__':
    main()
