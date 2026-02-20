#include "coloringInspector.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include <algorithm>
#include <sstream>

namespace {
Color oppositeColor(Color color) {
    return {
        static_cast<unsigned char>(255 - color.r),
        static_cast<unsigned char>(255 - color.g),
        static_cast<unsigned char>(255 - color.b),
        color.a
    };
}

FillPattern makeLargeDottedStyle(Color fillColor) {
    FillPattern style;
    style.type = FillPatternType::Dotted;
    style.size = 7.5f;
    style.useAccentColor = true;
    style.accentColor = oppositeColor(fillColor);
    return style;
}

FillPattern makeLargeStripedStyle(Color fillColor) {
    FillPattern style;
    style.type = FillPatternType::Striped;
    style.size = 5.f;
    style.useAccentColor = true;
    style.accentColor = oppositeColor(fillColor);
    return style;
}

bool isNativeMobilePlatform() {
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    return true;
#else
    return false;
#endif
}

Color resolveRegionPaletteColor(const Region& region, const std::vector<Color>& colorPalette, Color fallbackColor) {
    int colorIndex = region.getColor();
    if (region.hasColor() && colorIndex >= 0 && colorIndex < static_cast<int>(colorPalette.size())) {
        return colorPalette[colorIndex];
    }

    return fallbackColor;
}
}

void ColoringInspector::enterAnalysisMode() {
    analysisMode = true;
    clearAnalysisSelection();
}

void ColoringInspector::exitAnalysisMode() {
    analysisMode = false;
    clearAnalysisSelection();
}

void ColoringInspector::clearAnalysisSelection() {
    analysisInspectRegionId = -1;
    analysisHoverRegionId = -1;
}

bool ColoringInspector::isAnalysisMode() const {
    return analysisMode;
}

void ColoringInspector::updateAnalysis(const Mandala& mandala, const Camera2D& camera, bool pointerOverUi, bool isDraggingCamera) {
    if (!analysisMode) {
        return;
    }

    if (isNativeMobilePlatform()) {
        analysisHoverRegionId = -1;

        if (pointerOverUi) {
            return;
        }

        if (!isDraggingCamera && Input::IsPointerPressed()) {
            Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
            int tappedRegionId = getRegionIdAtWorldPosition(mandala, worldPos);
            if (tappedRegionId >= 0) {
                analysisInspectRegionId = tappedRegionId;
            }
        }
        return;
    }

    if (pointerOverUi) {
        analysisHoverRegionId = -1;
        return;
    }

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    analysisHoverRegionId = getRegionIdAtWorldPosition(mandala, worldPos);

    if (!isDraggingCamera && Input::IsPointerPressed() && analysisHoverRegionId >= 0) {
        analysisInspectRegionId = analysisHoverRegionId;
    }
}

void ColoringInspector::updateDebug(const Mandala& mandala, const Camera2D& camera, Camera2D& mutableCamera) {
    int previousInspectRegionId = debugInspectRegionId;

    if (IsKeyPressed(KEY_F3)) {
        debugAdjacencyMode = !debugAdjacencyMode;
        if (!debugAdjacencyMode) {
            debugInspectRegionId = -1;
            debugHoverRegionId = -1;
        }
    }

    if (!debugAdjacencyMode) {
        return;
    }

    int arrowDirection = 0;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP)) {
        arrowDirection = 1;
    } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_DOWN)) {
        arrowDirection = -1;
    }

    if (arrowDirection != 0) {
        const auto& regions = mandala.getRegions();
        std::vector<int> sortedIds = collectSortedRegionIds(regions);
        debugInspectRegionId = cycleRegionId(sortedIds, debugInspectRegionId, arrowDirection);
    }

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    debugHoverRegionId = getRegionIdAtWorldPosition(mandala, worldPos);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && debugHoverRegionId >= 0) {
        debugInspectRegionId = debugHoverRegionId;
    }

    if (IsKeyPressed(KEY_C)) {
        debugInspectRegionId = -1;
    }

    if (debugInspectRegionId >= 0 && debugHoverRegionId >= 0 && debugInspectRegionId != debugHoverRegionId) {
        if (IsKeyPressed(KEY_A)) {
            logAdjacencySuggestion(mandala, true, debugInspectRegionId, debugHoverRegionId);
        }
        if (IsKeyPressed(KEY_R)) {
            logAdjacencySuggestion(mandala, false, debugInspectRegionId, debugHoverRegionId);
        }
    }

    if (debugInspectRegionId >= 0 && debugInspectRegionId != previousInspectRegionId) {
        centerCameraOnRegion(mandala, debugInspectRegionId, mutableCamera);
    }
}

void ColoringInspector::drawAnalysisOverlay(const Mandala& mandala, const std::vector<Color>& colorPalette) const {
    if (!analysisMode) {
        return;
    }

    if (analysisInspectRegionId >= 0) {
        const Region* selectedRegion = mandala.getRegionById(analysisInspectRegionId);
        if (selectedRegion != nullptr) {
            Color selectedFill = resolveRegionPaletteColor(*selectedRegion, colorPalette, selectedRegion->getDefaultColor());
            selectedRegion->drawWithColor(selectedFill, Colors::DarkCyan, 6.0f,
                                          makeLargeStripedStyle(selectedFill));

            const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(analysisInspectRegionId);
            for (int neighborId : neighbors) {
                const Region* neighborRegion = mandala.getRegionById(neighborId);
                if (neighborRegion != nullptr) {
                    Color neighborFill = resolveRegionPaletteColor(*neighborRegion, colorPalette, neighborRegion->getDefaultColor());
                    neighborRegion->drawWithColor(neighborFill, Colors::Blue, 3.0f,
                                                  makeLargeDottedStyle(neighborFill));
                }
            }
        }
    }

    if (analysisHoverRegionId >= 0 && analysisHoverRegionId != analysisInspectRegionId) {
        const Region* hoverRegion = mandala.getRegionById(analysisHoverRegionId);
        if (hoverRegion != nullptr) {
            Color hoverFill = Fade(Colors::LightSkyBlue, 0.35f);
            hoverRegion->drawWithColor(hoverFill, Colors::DodgerBlue, 2.0f,
                                       makeLargeStripedStyle(hoverFill));
        }
    }
}

void ColoringInspector::drawDebugOverlay(const Mandala& mandala) const {
    if (!debugAdjacencyMode) {
        return;
    }

    if (debugInspectRegionId < 0) {
        return;
    }

    const Region* selectedRegion = mandala.getRegionById(debugInspectRegionId);
    if (selectedRegion == nullptr) {
        return;
    }

    selectedRegion->drawWithColor(Fade(Colors::Cyan, 0.45f), Colors::DarkCyan, 6.0f);

    const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
    for (int neighborId : neighbors) {
        const Region* neighborRegion = mandala.getRegionById(neighborId);
        if (neighborRegion == nullptr) {
            continue;
        }

        neighborRegion->drawWithColor(Fade(Colors::Blue, 0.30f), Colors::Blue, 4.0f);
    }

    if (debugHoverRegionId >= 0 && debugHoverRegionId != debugInspectRegionId) {
        const Region* hoverRegion = mandala.getRegionById(debugHoverRegionId);
        if (hoverRegion != nullptr) {
            hoverRegion->drawWithColor(Fade(Colors::LightSkyBlue, 0.35f), Colors::DodgerBlue, 2.0f);
        }
    }
}

void ColoringInspector::drawDebugInfoPanel(const Mandala& mandala, float uiScale) const {
    if (!debugAdjacencyMode) {
        return;
    }

    int infoX = static_cast<int>(15.0f * uiScale);
    int infoY = static_cast<int>(70.0f * uiScale);
    int infoW = static_cast<int>(760.0f * uiScale);
    int infoH = static_cast<int>(58.0f * uiScale);
    DrawRectangle(infoX, infoY, infoW, infoH, Fade(Colors::White, 0.85f));
    DrawRectangleLines(infoX, infoY, infoW, infoH, Colors::DarkGray);

    std::ostringstream info;
    info << "DEBUG ADJ: ON  |  Hover: " << debugHoverRegionId
         << "  |  Inspect (Right Click): " << debugInspectRegionId
         << "  |  Clear Inspect: C  |  A=Add  R=Remove";
    DrawText(info.str().c_str(), static_cast<int>(24.0f * uiScale), static_cast<int>(82.0f * uiScale),
             static_cast<int>(20.0f * uiScale), Colors::Black);

    if (debugInspectRegionId >= 0) {
        const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
        std::ostringstream neighborText;
        neighborText << "Neighbors(" << neighbors.size() << "): ";

        bool first = true;
        for (int id : neighbors) {
            if (!first) {
                neighborText << ", ";
            }
            neighborText << id;
            first = false;
        }

        DrawText(neighborText.str().c_str(), static_cast<int>(24.0f * uiScale), static_cast<int>(103.0f * uiScale),
                 static_cast<int>(18.0f * uiScale), Colors::DarkBlue);
    }
}

bool ColoringInspector::isDebugAdjacencyMode() const {
    return debugAdjacencyMode;
}

int ColoringInspector::getDebugInspectRegionId() const {
    return debugInspectRegionId;
}

int ColoringInspector::getDebugHoverRegionId() const {
    return debugHoverRegionId;
}

std::vector<int> ColoringInspector::collectSortedRegionIds(const std::vector<Region>& regions) {
    std::vector<int> ids;
    ids.reserve(regions.size());
    for (const auto& region : regions) {
        ids.push_back(region.getId());
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

int ColoringInspector::cycleRegionId(const std::vector<int>& sortedIds, int currentId, int direction) {
    if (sortedIds.empty()) {
        return -1;
    }

    if (currentId < 0) {
        return sortedIds.front();
    }

    auto it = std::lower_bound(sortedIds.begin(), sortedIds.end(), currentId);
    if (it == sortedIds.end() || *it != currentId) {
        if (direction > 0) {
            return (it == sortedIds.end()) ? sortedIds.front() : *it;
        }

        if (it == sortedIds.begin()) {
            return sortedIds.back();
        }

        --it;
        return *it;
    }

    int index = static_cast<int>(it - sortedIds.begin());
    int size = static_cast<int>(sortedIds.size());
    int nextIndex = (index + direction + size) % size;
    return sortedIds[nextIndex];
}

int ColoringInspector::getRegionIdAtWorldPosition(const Mandala& mandala, Vector2 worldPos) const {
    const auto& regions = mandala.getRegions();
    for (const auto& region : regions) {
        if (region.isPointInRegion(worldPos) && region.isColorable()) {
            return region.getId();
        }
    }

    return -1;
}

void ColoringInspector::centerCameraOnRegion(const Mandala& mandala, int regionId, Camera2D& camera) const {
    const Region* region = mandala.getRegionById(regionId);
    if (region == nullptr) {
        return;
    }

    camera.target = region->getCentroid();
}

void ColoringInspector::logAdjacencySuggestion(const Mandala& mandala, bool shouldExist, int regionA, int regionB) {
    int a = std::min(regionA, regionB);
    int b = std::max(regionA, regionB);
    std::pair<int, int> pair = {a, b};

    bool currentlyAdjacent = mandala.getAdjacencyGraph().areAdjacent(a, b);

    if (shouldExist) {
        debugSuggestedAdds.insert(pair);
        debugSuggestedRemoves.erase(pair);

        TraceLog(LOG_INFO, "[ADJ DEBUG] Suggest ADD (%d, %d)", a, b);
        if (currentlyAdjacent) {
            TraceLog(LOG_INFO, "[ADJ DEBUG] Already adjacent in current graph.");
        }
        TraceLog(LOG_INFO, "[ADJ DEBUG] Line to add: adjacencyGraph.addAdjacency(%d, %d);", a, b);
        return;
    }

    debugSuggestedRemoves.insert(pair);
    debugSuggestedAdds.erase(pair);

    TraceLog(LOG_INFO, "[ADJ DEBUG] Suggest REMOVE (%d, %d)", a, b);
    if (!currentlyAdjacent) {
        TraceLog(LOG_INFO, "[ADJ DEBUG] Pair not currently adjacent in graph.");
    }
    TraceLog(LOG_INFO, "[ADJ DEBUG] Line to remove from 3_adjacency.cpp: adjacencyGraph.addAdjacency(%d, %d);", a, b);
}
