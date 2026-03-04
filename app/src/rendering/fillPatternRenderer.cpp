#include "fillPatternRenderer.h"
#include "tesselator.h"
#include "raymath.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <unordered_map>

namespace {
constexpr float STRIPE_ANGLE_DEGREES = 45.0f;
constexpr float STRIPE_SPACING = 16.0f;
constexpr float STRIPE_THICKNESS = 5.0f;
constexpr float STRIPE_STEP = 6.0f;

constexpr float DOT_SPACING = 16.0f;
constexpr float DOT_RADIUS = 3.0f;

float clampPatternSize(float patternSize) {
    return Clamp(patternSize, 0.25f, 4.0f);
}

struct Bounds {
    float minX;
    float minY;
    float maxX;
    float maxY;
};

struct PatternAdaptation {
    float sizeMultiplier;
};

PatternAdaptation getPatternAdaptation(const std::vector<Vector2>& vertices);

PatternAdaptation getPatternAdaptationCached(const std::vector<Vector2>& vertices) {
    if (vertices.size() <= 8 || vertices.data() == nullptr) {
        return getPatternAdaptation(vertices);
    }

    static std::unordered_map<std::uint64_t, PatternAdaptation> cache;
    constexpr std::size_t MAX_CACHE_ENTRIES = 4096;

    const std::uint64_t pointerKey = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(vertices.data()));
    const std::uint64_t sizeKey = static_cast<std::uint64_t>(vertices.size()) * 1099511628211ULL;
    const std::uint64_t key = pointerKey ^ sizeKey;

    auto iterator = cache.find(key);
    if (iterator != cache.end()) {
        return iterator->second;
    }

    PatternAdaptation computed = getPatternAdaptation(vertices);
    if (cache.size() >= MAX_CACHE_ENTRIES) {
        cache.clear();
    }
    cache.emplace(key, computed);
    return computed;
}

Bounds getBounds(const std::vector<Vector2>& vertices) {
    Bounds bounds{0.0f, 0.0f, 0.0f, 0.0f};
    if (vertices.empty()) {
        return bounds;
    }

    bounds.minX = vertices.front().x;
    bounds.maxX = vertices.front().x;
    bounds.minY = vertices.front().y;
    bounds.maxY = vertices.front().y;

    for (const Vector2& vertex : vertices) {
        bounds.minX = std::min(bounds.minX, vertex.x);
        bounds.maxX = std::max(bounds.maxX, vertex.x);
        bounds.minY = std::min(bounds.minY, vertex.y);
        bounds.maxY = std::max(bounds.maxY, vertex.y);
    }

    return bounds;
}

float getSignedPolygonArea(const std::vector<Vector2>& vertices) {
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

PatternAdaptation getPatternAdaptation(const std::vector<Vector2>& vertices) {
    PatternAdaptation adaptation{1.0f};
    if (vertices.size() < 3) {
        return adaptation;
    }

    Bounds bounds = getBounds(vertices);
    float width = std::max(1.0f, bounds.maxX - bounds.minX);
    float height = std::max(1.0f, bounds.maxY - bounds.minY);
    float minDimension = std::min(width, height);

    float area = std::fabs(getSignedPolygonArea(vertices));
    float areaLengthScale = std::sqrt(std::max(area, 1.0f));

    float dimensionFactor = Clamp(minDimension / 120.0f, 0.38f, 1.0f);
    float areaFactor = Clamp(areaLengthScale / 95.0f, 0.38f, 1.0f);

    adaptation.sizeMultiplier = std::min(dimensionFactor, areaFactor);
    return adaptation;
}

bool isPointInPolygon(const std::vector<Vector2>& vertices, Vector2 point) {
    if (vertices.size() < 3) {
        return false;
    }

    int intersections = 0;
    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        Vector2 p1 = vertices[i];
        Vector2 p2 = vertices[(i + 1) % vertices.size()];

        if ((p1.y > point.y) != (p2.y > point.y)) {
            float xinters = (p2.x - p1.x) * (point.y - p1.y) / (p2.y - p1.y) + p1.x;
            if (point.x < xinters) {
                intersections++;
            }
        }
    }

    return (intersections % 2) == 1;
}

void drawSolidFill(const std::vector<Vector2>& vertices, Color fillColor) {
    TESStesselator* tess = tessNewTess(nullptr);
    if (!tess) {
        for (size_t i = 0; i < vertices.size() - 2; i++) {
            DrawTriangle(vertices[0], vertices[i + 1], vertices[i + 2], fillColor);
        }
        return;
    }

    std::vector<float> coords;
    coords.reserve(vertices.size() * 2);
    for (const Vector2& v : vertices) {
        coords.push_back(v.x);
        coords.push_back(v.y);
    }

    tessAddContour(tess, 2, coords.data(), sizeof(float) * 2, vertices.size());

    if (tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, nullptr)) {
        const float* verts = tessGetVertices(tess);
        const TESSindex* elems = tessGetElements(tess);
        const int nelems = tessGetElementCount(tess);

        for (int i = 0; i < nelems; i++) {
            const TESSindex* p = &elems[i * 3];
            if (p[0] != TESS_UNDEF && p[1] != TESS_UNDEF && p[2] != TESS_UNDEF) {
                Vector2 v0 = {verts[p[0] * 2], verts[p[0] * 2 + 1]};
                Vector2 v1 = {verts[p[1] * 2], verts[p[1] * 2 + 1]};
                Vector2 v2 = {verts[p[2] * 2], verts[p[2] * 2 + 1]};
                DrawTriangle(v0, v1, v2, fillColor);
            }
        }
    } else {
        for (size_t i = 0; i < vertices.size() - 2; i++) {
            DrawTriangle(vertices[0], vertices[i + 1], vertices[i + 2], fillColor);
        }
    }

    tessDeleteTess(tess);
}

void drawStripedFill(const std::vector<Vector2>& vertices, Color fillColor, Color stripeColor, float patternSize) {
    drawSolidFill(vertices, Fade(fillColor, 0.35f));

    PatternAdaptation adaptation = getPatternAdaptationCached(vertices);
    float sizeScale = clampPatternSize(patternSize) * adaptation.sizeMultiplier;
    float stripeSpacing = STRIPE_SPACING * sizeScale;
    float stripeThickness = std::max(1.0f, STRIPE_THICKNESS * sizeScale);
    float stripeStep = std::max(1.0f, STRIPE_STEP * sizeScale);

    Bounds bounds = getBounds(vertices);
    float angleRadians = STRIPE_ANGLE_DEGREES * DEG2RAD;
    Vector2 direction = {std::cos(angleRadians), std::sin(angleRadians)};
    Vector2 normal = {-direction.y, direction.x};

    std::array<Vector2, 4> corners = {{{bounds.minX, bounds.minY},
                                       {bounds.maxX, bounds.minY},
                                       {bounds.maxX, bounds.maxY},
                                       {bounds.minX, bounds.maxY}}};

    float minProjectionAlongDirection = direction.x * corners[0].x + direction.y * corners[0].y;
    float maxProjectionAlongDirection = minProjectionAlongDirection;
    float minProjectionAlongNormal = normal.x * corners[0].x + normal.y * corners[0].y;
    float maxProjectionAlongNormal = minProjectionAlongNormal;

    for (const Vector2& corner : corners) {
        float projectionAlongDirection = direction.x * corner.x + direction.y * corner.y;
        float projectionAlongNormal = normal.x * corner.x + normal.y * corner.y;
        minProjectionAlongDirection = std::min(minProjectionAlongDirection, projectionAlongDirection);
        maxProjectionAlongDirection = std::max(maxProjectionAlongDirection, projectionAlongDirection);
        minProjectionAlongNormal = std::min(minProjectionAlongNormal, projectionAlongNormal);
        maxProjectionAlongNormal = std::max(maxProjectionAlongNormal, projectionAlongNormal);
    }

        for (float normalOffset = minProjectionAlongNormal - stripeSpacing;
            normalOffset <= maxProjectionAlongNormal + stripeSpacing;
            normalOffset += stripeSpacing) {
        bool inRun = false;
        Vector2 runStart = {0.0f, 0.0f};
        Vector2 previousPoint = {0.0f, 0.0f};

           for (float along = minProjectionAlongDirection - stripeStep;
               along <= maxProjectionAlongDirection + stripeStep;
               along += stripeStep) {
            Vector2 samplePoint = {
                direction.x * along + normal.x * normalOffset,
                direction.y * along + normal.y * normalOffset,
            };

            bool inside = isPointInPolygon(vertices, samplePoint);
            if (inside && !inRun) {
                inRun = true;
                runStart = samplePoint;
            }

            if (!inside && inRun) {
                DrawLineEx(runStart, previousPoint, stripeThickness, stripeColor);
                inRun = false;
            }

            previousPoint = samplePoint;
        }

        if (inRun) {
            DrawLineEx(runStart, previousPoint, stripeThickness, stripeColor);
        }
    }
}

void drawDottedFill(const std::vector<Vector2>& vertices, Color fillColor, Color dotColor, float patternSize) {
    drawSolidFill(vertices, Fade(fillColor, 0.25f));

    PatternAdaptation adaptation = getPatternAdaptationCached(vertices);
    float sizeScale = clampPatternSize(patternSize) * adaptation.sizeMultiplier;
    float dotSpacing = DOT_SPACING * sizeScale;
    float dotRadius = std::max(1.0f, DOT_RADIUS * sizeScale);

    Bounds bounds = getBounds(vertices);
    int rowIndex = 0;

    for (float y = bounds.minY; y <= bounds.maxY; y += dotSpacing, rowIndex++) {
        float xOffset = (rowIndex % 2 == 0) ? 0.0f : dotSpacing * 0.5f;
        for (float x = bounds.minX + xOffset; x <= bounds.maxX; x += dotSpacing) {
            Vector2 center = {x, y};
            if (isPointInPolygon(vertices, center)) {
                DrawCircleV(center, dotRadius, dotColor);
            }
        }
    }
}

void drawInsetBorder(const std::vector<Vector2>& vertices, Color borderColor, float borderThickness) {
    if (vertices.size() < 2) {
        return;
    }

    const float signedArea = getSignedPolygonArea(vertices);
    const bool isCounterClockwise = signedArea > 0.0f;
    const float insetOffset = std::max(1.0f, borderThickness * 0.9f);

    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vector2 p1 = vertices[i];
        const Vector2 p2 = vertices[(i + 1) % vertices.size()];

        Vector2 edge = Vector2Subtract(p2, p1);
        const float edgeLength = Vector2Length(edge);
        if (edgeLength <= 0.0001f) {
            continue;
        }

        edge = Vector2Scale(edge, 1.0f / edgeLength);

        Vector2 leftNormal = {-edge.y, edge.x};
        Vector2 inwardNormal = isCounterClockwise ? leftNormal : Vector2Negate(leftNormal);
        Vector2 offset = Vector2Scale(inwardNormal, insetOffset);

        DrawLineEx(Vector2Add(p1, offset), Vector2Add(p2, offset), borderThickness, borderColor);
    }
}

void drawBorderedFill(const std::vector<Vector2>& vertices, Color fillColor, Color borderColor, float patternSize) {
    drawSolidFill(vertices, fillColor);

    PatternAdaptation adaptation = getPatternAdaptationCached(vertices);
    const float sizeScale = clampPatternSize(patternSize) * adaptation.sizeMultiplier;
    const float borderThickness = std::max(1.0f, 3.0f * sizeScale);
    drawInsetBorder(vertices, borderColor, borderThickness);
}
}

void FillPatternRenderer::drawPolygonFill(const std::vector<Vector2>& vertices, Color fillColor,
                                          FillPattern style) {
    if (vertices.size() < 3) {
        return;
    }

    Color patternColor = style.useAccentColor ? style.accentColor : fillColor;

    switch (style.type) {
        case FillPatternType::Striped:
            drawStripedFill(vertices, fillColor, patternColor, style.size);
            return;
        case FillPatternType::Dotted:
            drawDottedFill(vertices, fillColor, patternColor, style.size);
            return;
        case FillPatternType::Bordered:
            drawBorderedFill(vertices, fillColor, patternColor, style.size);
            return;
        case FillPatternType::Solid:
        default:
            drawSolidFill(vertices, fillColor);
            return;
    }
}

void FillPatternRenderer::drawRectangleFill(Rectangle bounds, Color fillColor,
                                            FillPattern style) {
    if (style.type == FillPatternType::Solid) {
        DrawRectangleRec(bounds, fillColor);
        return;
    }

    float minX = std::min(bounds.x, bounds.x + bounds.width);
    float maxX = std::max(bounds.x, bounds.x + bounds.width);
    float minY = std::min(bounds.y, bounds.y + bounds.height);
    float maxY = std::max(bounds.y, bounds.y + bounds.height);

    std::vector<Vector2> vertices = {
        {minX, minY},
        {maxX, minY},
        {maxX, maxY},
        {minX, maxY},
    };

    drawPolygonFill(vertices, fillColor, style);
}
