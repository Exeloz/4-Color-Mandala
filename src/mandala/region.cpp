#include "region.h"
#include "../ui/colors.h"
#include "tesselator.h"
#include <cmath>
#include <algorithm>

Region::Region(int id, const std::vector<Vector2>& vertices)
        : id(id),
            vertices(vertices),
            colorIndex(-1),
            colored(false),
            defaultColor(Colors::None),
            colorable(true) {}

int Region::getId() const {
    return id;
}

void Region::setColor(int colorIndex) {
    if (!colorable) {
        return;
    }
    this->colorIndex = colorIndex;
    colored = (colorIndex >= 0);
}

int Region::getColor() const {
    return colorIndex;
}

bool Region::hasColor() const {
    return colored;
}

void Region::setDefaultColor(Color color) {
    defaultColor = color;
}

Color Region::getDefaultColor() const {
    return defaultColor;
}

void Region::setColorable(bool canColor) {
    colorable = canColor;
}

bool Region::isColorable() const {
    return colorable;
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

    Color fillColor = defaultColor;
    if (colored && colorIndex >= 0 && colorIndex < static_cast<int>(colorPalette.size())) {
        fillColor = colorPalette[colorIndex];
    }

    TESStesselator* tess = tessNewTess(nullptr);
    if (!tess) {
        for (size_t i = 0; i < vertices.size() - 2; i++) {
            DrawTriangle(vertices[0], vertices[i + 1], vertices[i + 2], fillColor);
        }
    } else {
        std::vector<float> coords;
        coords.reserve(vertices.size() * 2);
        for (const auto& v : vertices) {
            coords.push_back(v.x);
            coords.push_back(v.y);
        }

        tessAddContour(tess, 2, coords.data(), sizeof(float) * 2, vertices.size());

        if (tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, nullptr)) {
            const float* verts = tessGetVertices(tess);
            const TESSindex* elems = tessGetElements(tess);
            const int nelems = tessGetElementCount(tess);
            const int nvp = 3; // vertices per polygon (triangles)

            for (int i = 0; i < nelems; i++) {
                const TESSindex* p = &elems[i * nvp];
                if (p[0] != TESS_UNDEF && p[1] != TESS_UNDEF && p[2] != TESS_UNDEF) {
                    Vector2 v0 = {verts[p[0] * 2], verts[p[0] * 2 + 1]};
                    Vector2 v1 = {verts[p[1] * 2], verts[p[1] * 2 + 1]};
                    Vector2 v2 = {verts[p[2] * 2], verts[p[2] * 2 + 1]};
                    DrawTriangle(v0, v1, v2, fillColor);
                }
            }
        }

        tessDeleteTess(tess);
    }

    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        Vector2 p1 = vertices[i];
        Vector2 p2 = vertices[(i + 1) % vertices.size()];
        DrawLineEx(p1, p2, 5, Colors::Black);
    }

    Vector2 centroid = {0, 0};
    for (const auto& v : vertices) {
        centroid.x += v.x;
        centroid.y += v.y;
    }
    centroid.x /= vertices.size();
    centroid.y /= vertices.size();
}

