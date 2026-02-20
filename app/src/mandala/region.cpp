#include "region.h"
#include "../ui/colors.h"
#include <cmath>
#include <algorithm>

Region::Region(int id, const std::vector<Vector2>& vertices)
        : id(id),
            vertices(vertices),
            colorIndex(-1),
            colored(false),
            defaultColor(Colors::None),
            fillPattern(FillPattern::Solid),
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

void Region::drawWithColor(Color fillColor, Color borderColor, float borderWidth, FillPattern pattern) const {
    if (vertices.size() < 3) return;

    RegionFillStyleFactory::getStyle(pattern).drawFill(vertices, fillColor);

    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        Vector2 p1 = vertices[i];
        Vector2 p2 = vertices[(i + 1) % vertices.size()];
        DrawLineEx(p1, p2, borderWidth, borderColor);
    }
}

