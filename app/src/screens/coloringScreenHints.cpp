#include "coloringScreen.h"

#include "raymath.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace {
float pointSegmentDistanceSquared(Vector2 p, Vector2 a, Vector2 b) {
    const float abx = b.x - a.x;
    const float aby = b.y - a.y;
    const float apx = p.x - a.x;
    const float apy = p.y - a.y;
    const float abLenSq = abx * abx + aby * aby;

    if (abLenSq <= 1e-8f) {
        const float dx = p.x - a.x;
        const float dy = p.y - a.y;
        return dx * dx + dy * dy;
    }

    float t = (apx * abx + apy * aby) / abLenSq;
    t = Clamp(t, 0.0f, 1.0f);
    const float qx = a.x + t * abx;
    const float qy = a.y + t * aby;
    const float dx = p.x - qx;
    const float dy = p.y - qy;
    return dx * dx + dy * dy;
}

float regionAreaAbs(const Region& region) {
    const auto& vertices = region.getVertices();
    if (vertices.size() < 3) {
        return 0.0f;
    }

    float twiceArea = 0.0f;
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        const Vector2& a = vertices[i];
        const Vector2& b = vertices[(i + 1) % vertices.size()];
        twiceArea += (a.x * b.y) - (b.x * a.y);
    }
    return std::fabs(twiceArea) * 0.5f;
}

bool isPointInsideConflictingRegion(const Mandala& mandala, const Region& currentRegion, Vector2 point) {
    const float currentArea = regionAreaAbs(currentRegion);

    for (const Region& other : mandala.getRegions()) {
        if (other.getId() == currentRegion.getId()) {
            continue;
        }

        if (!other.isPointInRegion(point)) {
            continue;
        }

        // If point is also in a smaller nested region, that smaller region owns the label area.
        // Larger enclosing regions are allowed so child regions can still place labels inside themselves.
        const float otherArea = regionAreaAbs(other);
        if (otherArea + 1e-3f < currentArea) {
            return true;
        }
    }
    return false;
}

Vector2 findNonOverlappingLabelPoint(const Mandala& mandala, const Region& region) {
    Vector2 basePoint = region.getInteriorPoint();
    if (!isPointInsideConflictingRegion(mandala, region, basePoint)) {
        return basePoint;
    }

    const auto& vertices = region.getVertices();
    if (vertices.empty()) {
        return basePoint;
    }

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;
    for (const Vector2& v : vertices) {
        minX = std::min(minX, v.x);
        minY = std::min(minY, v.y);
        maxX = std::max(maxX, v.x);
        maxY = std::max(maxY, v.y);
    }

    const float spanX = std::max(1e-5f, maxX - minX);
    const float spanY = std::max(1e-5f, maxY - minY);
    const int gridSteps = 14;

    bool found = false;
    Vector2 best = basePoint;
    float bestScore = -1.0f;

    for (int gy = 0; gy <= gridSteps; ++gy) {
        const float y = minY + (spanY * static_cast<float>(gy) / static_cast<float>(gridSteps));
        for (int gx = 0; gx <= gridSteps; ++gx) {
            const float x = minX + (spanX * static_cast<float>(gx) / static_cast<float>(gridSteps));
            const Vector2 candidate{x, y};

            if (!region.isPointInRegion(candidate)) {
                continue;
            }
            if (isPointInsideConflictingRegion(mandala, region, candidate)) {
                continue;
            }

            float minEdgeDistSq = FLT_MAX;
            for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
                const Vector2 a = vertices[i];
                const Vector2 b = vertices[(i + 1) % vertices.size()];
                minEdgeDistSq = std::min(minEdgeDistSq, pointSegmentDistanceSquared(candidate, a, b));
            }

            if (!found || minEdgeDistSq > bestScore) {
                found = true;
                best = candidate;
                bestScore = minEdgeDistSq;
            }
        }
    }

    return found ? best : basePoint;
}
}

int ColoringScreen::getLastColorHintRemainingCount(int regionId) const {
    if (!mandala || !mandala->hasLastColorHintData() || !mandala->isLastColorHintTrackedRegion(regionId)) {
        return 0;
    }

    const int targetColor = mandala->getLastColorHintTargetColor();
    if (targetColor <= 0) {
        return 0;
    }

    int remainingCount = 0;
    const AdjacencyGraph& graph = mandala->getAdjacencyGraph();
    const std::set<int>& adjacent = graph.getAdjacentRegions(regionId);
    for (int adjacentId : adjacent) {
        if (mandala->getLastColorHintSolutionColor(adjacentId) != targetColor) {
            continue;
        }

        const Region* adjacentRegion = mandala->getRegionById(adjacentId);
        if (adjacentRegion == nullptr) {
            continue;
        }

        if (adjacentRegion->getColor() != targetColor) {
            remainingCount++;
        }
    }

    return remainingCount;
}

bool ColoringScreen::isLastColorHintRuleSatisfied() const {
    if (!mandala || !mandala->hasLastColorHintData()) {
        return true;
    }

    for (const Region& region : mandala->getRegions()) {
        if (!region.isColorable()) {
            continue;
        }

        const int regionId = region.getId();
        if (!mandala->isLastColorHintTrackedRegion(regionId)) {
            continue;
        }

        if (getLastColorHintRemainingCount(regionId) > 0) {
            return false;
        }
    }

    return true;
}

void ColoringScreen::drawLastColorHintOverlay() const {
    if (!mandala || !mandala->hasLastColorHintData()) {
        return;
    }

    const float safeZoom = std::max(0.05f, camera.zoom);
    const int fontSize = std::max(12, static_cast<int>(18.0f / safeZoom));
    const float circleRadius = std::max(10.0f, 14.0f / safeZoom);

    for (const Region& region : mandala->getRegions()) {
        if (!region.isColorable()) {
            continue;
        }

        const int regionId = region.getId();
        if (!mandala->isLastColorHintTrackedRegion(regionId)) {
            continue;
        }

        const int remainingCount = getLastColorHintRemainingCount(regionId);
        if (remainingCount <= 0) {
            continue;
        }

        Vector2 center;
        const auto cached = lastColorHintLabelCache.find(regionId);
        if (cached != lastColorHintLabelCache.end()) {
            center = cached->second;
        } else {
            center = findNonOverlappingLabelPoint(*mandala, region);
            lastColorHintLabelCache.emplace(regionId, center);
        }

        const std::string label = std::to_string(remainingCount);
        const int textWidth = MeasureText(label.c_str(), fontSize);

        DrawCircleV(center, circleRadius, Color{22, 33, 44, 190});
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), circleRadius, Color{230, 236, 245, 220});
        DrawText(label.c_str(),
                 static_cast<int>(center.x - (textWidth * 0.5f)),
                 static_cast<int>(center.y - (fontSize * 0.5f)),
                 fontSize,
                 Color{255, 255, 255, 255});
    }
}
