import json

class Polygon:
    def __init__(self, points, closed = True):
        self.points = points
        self.closed = closed

    def to_svg(self):
        svg = "<polygon points=\""
        for point in self.points:
            svg += f"{point[0]},{point[1]} "
        svg += "\" style=\"fill:white;stroke:black;stroke-width:1\" />"
        return svg

with open('test1.json') as f:
    svg = json.load(f)
    
    polygons = []
    for polygon in svg:
        polygons.append(Polygon(polygon["points"], polygon["closed"]))

    new_svg = ["""<?xml version="1.0" encoding="UTF-8"?><svg id="a" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 800 600.7">
"""]
    
    for polygon in polygons:
        new_svg.append(polygon.to_svg() + "\n")

    new_svg.append("</svg>")

with open("test2.svg", "w") as f:
    f.writelines(new_svg)