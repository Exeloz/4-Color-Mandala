#include "region.h"
#include "../ui/colors.h"
#include <tesselator.h>
#include <cmath>
#include <algorithm>

Region::Region(int id, const std::vector<Vector2>& vertices)
        : id(id),
            vertices(vertices),
            triangleVertices(),
            colorIndex(-1),
            colored(false),
            defaultColor(Colors::None),
            fillPattern(),
            colorable(true) {
    buildTriangleCache();
}

int Region::getId() const {
    return id;
}

void Region::setColor(int colorIndex) {
    if (!colorable) {
        return;
    }

    const int normalizedColorIndex = (colorIndex <= 0) ? -1 : colorIndex;
    this->colorIndex = normalizedColorIndex;
    colored = (normalizedColorIndex >= 0);
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

void Region::setFillPattern(FillPattern pattern) {
    fillPattern = pattern;
}

FillPattern Region::getFillPattern() const {
    return fillPattern;
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

Vector2 Region::getCentroid() const {
    if (vertices.empty()) {
        return {0.0f, 0.0f};
    }

    if (vertices.size() == 1) {
        return vertices[0];
    }

    float twiceArea = 0.0f;
    float centroidX = 0.0f;
    float centroidY = 0.0f;

    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        const Vector2& current = vertices[i];
        const Vector2& next = vertices[(i + 1) % vertices.size()];
        float cross = (current.x * next.y) - (next.x * current.y);
        twiceArea += cross;
        centroidX += (current.x + next.x) * cross;
        centroidY += (current.y + next.y) * cross;
    }

    if (std::fabs(twiceArea) < 1e-6f) {
        Vector2 average = {0.0f, 0.0f};
        for (const auto& vertex : vertices) {
            average.x += vertex.x;
            average.y += vertex.y;
        }

        float inverseCount = 1.0f / static_cast<float>(vertices.size());
        return {average.x * inverseCount, average.y * inverseCount};
    }

    float scale = 1.0f / (3.0f * twiceArea);
    return {centroidX * scale, centroidY * scale};
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

void Region::draw(const std::vector<Color>& colorPalette, bool ignoreColoring) const {
    if (vertices.size() < 3) return;

    Color fillColor = defaultColor;
    if (!ignoreColoring && colored && colorIndex >= 0 && colorIndex < static_cast<int>(colorPalette.size())) {
        fillColor = colorPalette[colorIndex];
    }

    drawWithColor(fillColor, Colors::Black, 5.0f, fillPattern);
}

void Region::drawWithColor(Color fillColor, Color borderColor, float borderWidth,
                           FillPattern pattern) const {
    if (vertices.size() < 3) return;

    if (pattern.type == FillPatternType::Solid && !triangleVertices.empty()) {
        drawCachedSolidFill(fillColor);
    } else {
        RegionFillStyleFactory::getStyle(pattern.type).drawFill(
            vertices,
            fillColor,
            pattern);
    }

    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        Vector2 p1 = vertices[i];
        Vector2 p2 = vertices[(i + 1) % vertices.size()];
        DrawLineEx(p1, p2, borderWidth, borderColor);
    }
}

void Region::buildTriangleCache() {
    triangleVertices.clear();
    if (vertices.size() < 3) {
        return;
    }

    TESStesselator* tess = tessNewTess(nullptr);
    if (!tess) {
        triangleVertices.reserve((vertices.size() - 2) * 3);
        for (size_t i = 0; i + 2 < vertices.size(); ++i) {
            triangleVertices.push_back(vertices[0]);
            triangleVertices.push_back(vertices[i + 1]);
            triangleVertices.push_back(vertices[i + 2]);
        }
        return;
    }

    std::vector<float> coords;
    coords.reserve(vertices.size() * 2);
    for (const Vector2& v : vertices) {
        coords.push_back(v.x);
        coords.push_back(v.y);
    }

    tessAddContour(tess, 2, coords.data(), sizeof(float) * 2, static_cast<int>(vertices.size()));

    if (tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, nullptr)) {
        const float* verts = tessGetVertices(tess);
        const TESSindex* elems = tessGetElements(tess);
        const int nelems = tessGetElementCount(tess);

        triangleVertices.reserve(static_cast<size_t>(nelems) * 3);
        for (int i = 0; i < nelems; ++i) {
            const TESSindex* p = &elems[i * 3];
            if (p[0] == TESS_UNDEF || p[1] == TESS_UNDEF || p[2] == TESS_UNDEF) {
                continue;
            }

            triangleVertices.push_back(Vector2{verts[p[0] * 2], verts[p[0] * 2 + 1]});
            triangleVertices.push_back(Vector2{verts[p[1] * 2], verts[p[1] * 2 + 1]});
            triangleVertices.push_back(Vector2{verts[p[2] * 2], verts[p[2] * 2 + 1]});
        }
    }

    tessDeleteTess(tess);

    if (triangleVertices.empty()) {
        triangleVertices.reserve((vertices.size() - 2) * 3);
        for (size_t i = 0; i + 2 < vertices.size(); ++i) {
            triangleVertices.push_back(vertices[0]);
            triangleVertices.push_back(vertices[i + 1]);
            triangleVertices.push_back(vertices[i + 2]);
        }
    }
}

void Region::drawCachedSolidFill(Color fillColor) const {
    for (size_t i = 0; i + 2 < triangleVertices.size(); i += 3) {
        DrawTriangle(triangleVertices[i], triangleVertices[i + 1], triangleVertices[i + 2], fillColor);
    }
}

