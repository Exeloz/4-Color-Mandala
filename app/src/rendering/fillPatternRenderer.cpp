#include "fillPatternRenderer.h"
#include "tesselator.h"
#include "raymath.h"
#include <algorithm>
#include <array>
#include <cmath>

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

    float sizeScale = clampPatternSize(patternSize);
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

    float sizeScale = clampPatternSize(patternSize);
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
