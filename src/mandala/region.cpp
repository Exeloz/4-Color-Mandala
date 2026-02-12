#include "region.h"
#include <cmath>
#include <algorithm>

Region::Region(int id, const std::vector<Vector2>& vertices)
    : id(id), vertices(vertices), colorIndex(-1), colored(false) {}

int Region::getId() const {
    return id;
}

void Region::setColor(int colorIndex) {
    this->colorIndex = colorIndex;
    colored = true;
}

int Region::getColor() const {
    return colorIndex;
}

bool Region::hasColor() const {
    return colored;
}

const std::vector<Vector2>& Region::getVertices() const {
    return vertices;
}

bool Region::isPointInRegion(Vector2 point) const {
    int n = vertices.size();
    if (n < 3) return false;

    int intersections = 0;
    for (int i = 0; i < n; i++) {
        Vector2 p1 = vertices[i];
        Vector2 p2 = vertices[(i + 1) % n];

        if ((p1.y > point.y) != (p2.y > point.y)) {
            float xinters = (p2.x - p1.x) * (point.y - p1.y) / (p2.y - p1.y) + p1.x;
            if (point.x < xinters) {
                intersections++;
            }
        }
    }
    return intersections % 2 == 1;
}

void Region::draw(const std::vector<Color>& colorPalette) const {
    if (vertices.size() < 3) return;

    Color fillColor = {200, 200, 200, 255};
    if (colored && colorIndex >= 0 && colorIndex < static_cast<int>(colorPalette.size())) {
        fillColor = colorPalette[colorIndex];
    }

    for (size_t i = 0; i < vertices.size() - 2; i++) {
        DrawTriangle(vertices[0], vertices[i + 1], vertices[i + 2], fillColor);
    }

    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        Vector2 p1 = vertices[i];
        Vector2 p2 = vertices[(i + 1) % vertices.size()];
        DrawLineEx(p1, p2, 2, {0, 0, 0, 255});
    }

    Vector2 centroid = {0, 0};
    for (const auto& v : vertices) {
        centroid.x += v.x;
        centroid.y += v.y;
    }
    centroid.x /= vertices.size();
    centroid.y /= vertices.size();

    DrawCircleV(centroid, 3, {0, 0, 0, 255});
}

