#include "mandalaDatabaseAssetJson.h"

#include "../ui/colors.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

namespace {
bool parseColorFromString(const std::string& value, Color& output) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char c : value) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (normalized == "black") {
        output = Colors::Black;
        return true;
    }
    if (normalized == "white") {
        output = Colors::White;
        return true;
    }
    if (normalized == "transparent") {
        output = Colors::Transparent;
        return true;
    }
    if (normalized == "none") {
        output = Colors::None;
        return true;
    }

    return false;
}

bool toColorChannel(const JsonValue& value, unsigned char& channel) {
    if (!value.isNumber()) {
        return false;
    }

    const double raw = value.numberValue;
    if (raw < 0.0 || raw > 255.0) {
        return false;
    }

    channel = static_cast<unsigned char>(raw);
    return true;
}

bool arePointsNear(const Vector2& a, const Vector2& b, float epsilon = 0.0001f) {
    return std::fabs(a.x - b.x) <= epsilon && std::fabs(a.y - b.y) <= epsilon;
}

float computeSignedArea(const std::vector<Vector2>& vertices) {
    if (vertices.size() < 3) {
        return 0.0f;
    }

    float areaTimesTwo = 0.0f;
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vector2& current = vertices[i];
        const Vector2& next = vertices[(i + 1) % vertices.size()];
        areaTimesTwo += current.x * next.y - next.x * current.y;
    }
    return areaTimesTwo * 0.5f;
}

void normalizePolygonForTessellation(std::vector<Vector2>& vertices) {
    if (vertices.empty()) {
        return;
    }

    std::vector<Vector2> cleaned;
    cleaned.reserve(vertices.size());
    for (const Vector2& vertex : vertices) {
        if (cleaned.empty() || !arePointsNear(cleaned.back(), vertex)) {
            cleaned.push_back(vertex);
        }
    }

    if (cleaned.size() >= 2 && arePointsNear(cleaned.front(), cleaned.back())) {
        cleaned.pop_back();
    }

    vertices = std::move(cleaned);
    if (vertices.size() < 3) {
        return;
    }

    if (computeSignedArea(vertices) > 0.0f) {
        std::reverse(vertices.begin(), vertices.end());
    }
}
}

bool parseColorField(const JsonValue& value, Color& output) {
    if (value.isString()) {
        return parseColorFromString(value.stringValue, output);
    }

    if (value.isArray()) {
        if (value.arrayValue.size() < 3) {
            return false;
        }

        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 255;

        if (!toColorChannel(value.arrayValue[0], r)
            || !toColorChannel(value.arrayValue[1], g)
            || !toColorChannel(value.arrayValue[2], b)) {
            return false;
        }

        if (value.arrayValue.size() >= 4 && !toColorChannel(value.arrayValue[3], a)) {
            return false;
        }

        output = Color{r, g, b, a};
        return true;
    }

    if (value.isObject()) {
        const JsonValue* red = value.get("r");
        const JsonValue* green = value.get("g");
        const JsonValue* blue = value.get("b");
        const JsonValue* alpha = value.get("a");
        if (red == nullptr || green == nullptr || blue == nullptr) {
            return false;
        }

        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 255;
        if (!toColorChannel(*red, r) || !toColorChannel(*green, g) || !toColorChannel(*blue, b)) {
            return false;
        }
        if (alpha != nullptr && !toColorChannel(*alpha, a)) {
            return false;
        }

        output = Color{r, g, b, a};
        return true;
    }

    return false;
}

bool parseRegionVertices(const JsonValue& pointsValue, std::vector<Vector2>& vertices) {
    if (!pointsValue.isArray()) {
        return false;
    }

    vertices.clear();
    vertices.reserve(pointsValue.arrayValue.size());

    for (const JsonValue& pointValue : pointsValue.arrayValue) {
        if (!pointValue.isArray() || pointValue.arrayValue.size() < 2) {
            return false;
        }

        const JsonValue& xValue = pointValue.arrayValue[0];
        const JsonValue& yValue = pointValue.arrayValue[1];
        if (!xValue.isNumber() || !yValue.isNumber()) {
            return false;
        }

        vertices.push_back(Vector2{
            static_cast<float>(xValue.numberValue),
            static_cast<float>(yValue.numberValue),
        });
    }

    normalizePolygonForTessellation(vertices);
    return vertices.size() >= 3;
}
