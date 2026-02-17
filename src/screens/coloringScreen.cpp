#include "coloringScreen.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>
#include <utility>
#include <sstream>

namespace {
    constexpr float SCREEN_CENTER_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
    constexpr float MIN_ZOOM = 0.02f;
    constexpr float MAX_ZOOM = 4.0f;
    constexpr float ZOOM_STEP = 0.01f;

    std::vector<int> collectSortedRegionIds(const std::vector<Region>& regions) {
        std::vector<int> ids;
        ids.reserve(regions.size());
        for (const auto& region : regions) {
            ids.push_back(region.getId());
        }

        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        return ids;
    }

    int cycleRegionId(const std::vector<int>& sortedIds, int currentId, int direction) {
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
}

ColoringScreen::ColoringScreen(std::shared_ptr<Mandala> mandala, const std::vector<Color>& customPaletteColors)
        : mandala(mandala), colorPalette(), colorButtons(), backButton(20, 20, 100, 50, "BACK"),
            analysisButton(590, 20, 190, 50, "ANALYSIS"),
            analysisCloseButton(590, 20, 190, 50, "EXIT ANALYSIS"),
            analysisClearButton(710, 80, 70, 44, "CLEAR"),
            gameWon(false), returnRequested(false), analysisMode(false), analysisInspectRegionId(-1),
            analysisHoverRegionId(-1), camera{}, zoom(1.0f), isPanning(false),
            lastPanPointer{},
            debugAdjacencyMode(false), debugInspectRegionId(-1), debugHoverRegionId(-1),
            debugSuggestedAdds(), debugSuggestedRemoves() {

        camera.target = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
        camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
        camera.rotation = 0.0f;
        camera.zoom = zoom;

    if (!customPaletteColors.empty()) {
        colorPalette.setColors(customPaletteColors);
    }
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        float x = 50 + i * 130;
        float y = 550;
        colorButtons.emplace_back(x, y, 100, 40, "");
    }
}

void ColoringScreen::update(float deltaTime) {
    layoutTopButtons();
    updateZoom();
    updatePan();

    backButton.update();
    if (backButton.isClicked()) {
        returnRequested = true;
        return;
    }

    for (int i = 0; i < static_cast<int>(colorButtons.size()); i++) {
        colorButtons[i].update();
        if (colorButtons[i].isClicked()) {
            colorPalette.setSelectedColorIndex(i);
        }
    }

    updateAnalysisOverlay();
    updateDebugOverlay();
    if (!analysisMode) {
        handleColorSelection();
    }

    if (mandala->isValidColoring()) {
        gameWon = true;
    }
}

void ColoringScreen::handleColorSelection() {
    if (!isPanning && Input::IsPointerPressed()) {
        Vector2 pointerPos = Input::GetPointerPosition();
        if (isPointerOverUi(pointerPos)) {
            return;
        }

        Vector2 worldPos = GetScreenToWorld2D(pointerPos, camera);
        Region* region = mandala->getRegionAtPoint(worldPos);
        if (region != nullptr) {
            region->setColor(colorPalette.getSelectedColorIndex());
        }
    }
}

void ColoringScreen::updatePan() {
    bool shouldPan = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
                     (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_BUTTON_LEFT));

    Vector2 pointerPos = Input::GetPointerPosition();

    if (shouldPan) {
        if (!isPanning) {
            isPanning = true;
            lastPanPointer = pointerPos;
            return;
        }

        Vector2 delta = Vector2Subtract(pointerPos, lastPanPointer);
        camera.target = Vector2Subtract(camera.target, Vector2Scale(delta, 1.0f / camera.zoom));
        lastPanPointer = pointerPos;
    } else {
        isPanning = false;
    }
}

void ColoringScreen::updateZoom() {
    camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};

    float zoomDelta = 0.0f;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        zoomDelta += wheel * ZOOM_STEP;
    }

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    if (IsGestureDetected(GESTURE_PINCH_IN)) {
        zoomDelta -= ZOOM_STEP;
    }
    if (IsGestureDetected(GESTURE_PINCH_OUT)) {
        zoomDelta += ZOOM_STEP;
    }
#endif

    if (IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT)) {
        zoomDelta -= ZOOM_STEP * GetFrameTime();
    }
    if (IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD)) {
        zoomDelta += ZOOM_STEP * GetFrameTime();
    }

    if (zoomDelta != 0.0f) {
        Vector2 pointerPos = Input::GetPointerPosition();
        Vector2 worldBeforeZoom = GetScreenToWorld2D(pointerPos, camera);

        zoom = Clamp(zoom + zoomDelta, MIN_ZOOM, MAX_ZOOM);
        camera.zoom = zoom;

        Vector2 worldAfterZoom = GetScreenToWorld2D(pointerPos, camera);
        camera.target = Vector2Add(camera.target, Vector2Subtract(worldBeforeZoom, worldAfterZoom));
    }
}

void ColoringScreen::draw() {
    ClearBackground(Colors::LightBlue);
    
    DrawText(mandala->getName().c_str(), 150, 20, 30, Colors::Black);

    BeginMode2D(camera);
    mandala->draw(colorPalette.getColors());
    drawAnalysisOverlay();
    drawDebugOverlay();
    EndMode2D();

    if (analysisMode) {
        DrawRectangle(15, 70, 560, 58, Fade(Colors::White, 0.85f));
        DrawRectangleLines(15, 70, 560, 58, Colors::DarkGray);

        std::ostringstream info;
        info << "ANALYSIS MODE  |  Tap region to inspect  |  Hover: " << analysisHoverRegionId
             << "  |  Inspect: " << analysisInspectRegionId;
        DrawText(info.str().c_str(), 24, 82, 20, Colors::Black);

        if (analysisInspectRegionId >= 0) {
            const auto& neighbors = mandala->getAdjacencyGraph().getAdjacentRegions(analysisInspectRegionId);
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

            DrawText(neighborText.str().c_str(), 24, 103, 18, Colors::DarkBlue);
        }
    }

    if (debugAdjacencyMode) {
        DrawRectangle(15, 70, 760, 58, Fade(Colors::White, 0.85f));
        DrawRectangleLines(15, 70, 760, 58, Colors::DarkGray);

        std::ostringstream info;
        info << "DEBUG ADJ: ON  |  Hover: " << debugHoverRegionId
             << "  |  Inspect (Right Click): " << debugInspectRegionId
               << "  |  Clear Inspect: C  |  A=Add  R=Remove";
        DrawText(info.str().c_str(), 24, 82, 20, Colors::Black);

        if (debugInspectRegionId >= 0) {
            const auto& neighbors = mandala->getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
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

            DrawText(neighborText.str().c_str(), 24, 103, 18, Colors::DarkBlue);
        }
    }

    drawColorPalette();
    backButton.draw();
    if (analysisMode) {
        analysisCloseButton.draw();
        analysisClearButton.draw();
    } else {
        analysisButton.draw();
    }
}

void ColoringScreen::drawColorPalette() {
    DrawText("Colors:", 50, 530, 15, Colors::Black);
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        float x = 50 + i * 130;
        float y = 550;
        Color color = colorPalette.getColor(i);
        
        DrawRectangle(x, y, 100, 40, color);
        
        if (i == colorPalette.getSelectedColorIndex()) {
            DrawRectangleLinesEx({x, y, 100, 40}, 4, Colors::Black);
        } else {
            DrawRectangleLinesEx({x, y, 100, 40}, 1, Colors::Gray);
        }
    }
}

bool ColoringScreen::isGameWon() const {
    return gameWon;
}

bool ColoringScreen::shouldReturnToSelection() const {
    return returnRequested;
}

void ColoringScreen::updateAnalysisOverlay() {
    if (!analysisMode) {
        analysisButton.update();
        if (analysisButton.isClicked()) {
            analysisMode = true;
            analysisInspectRegionId = -1;
            analysisHoverRegionId = -1;
        }
        return;
    }

    analysisCloseButton.update();
    analysisClearButton.update();

    if (analysisCloseButton.isClicked()) {
        analysisMode = false;
        analysisInspectRegionId = -1;
        analysisHoverRegionId = -1;
        return;
    }

    if (analysisClearButton.isClicked()) {
        analysisInspectRegionId = -1;
        analysisHoverRegionId = -1;
        return;
    }

    Vector2 pointerPos = Input::GetPointerPosition();
    if (isPointerOverUi(pointerPos)) {
        analysisHoverRegionId = -1;
        return;
    }

    Vector2 worldPos = GetScreenToWorld2D(pointerPos, camera);
    analysisHoverRegionId = getRegionIdAtWorldPosition(worldPos);

    if (!isPanning && Input::IsPointerPressed() && analysisHoverRegionId >= 0) {
        analysisInspectRegionId = analysisHoverRegionId;
    }
}

void ColoringScreen::updateDebugOverlay() {
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
        const auto& regions = mandala->getRegions();
        std::vector<int> sortedIds = collectSortedRegionIds(regions);
        debugInspectRegionId = cycleRegionId(sortedIds, debugInspectRegionId, arrowDirection);
    }

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    debugHoverRegionId = getRegionIdAtWorldPosition(worldPos);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && debugHoverRegionId >= 0) {
        debugInspectRegionId = debugHoverRegionId;
    }

    if (IsKeyPressed(KEY_C)) {
        debugInspectRegionId = -1;
    }

    if (debugInspectRegionId >= 0 && debugHoverRegionId >= 0 && debugInspectRegionId != debugHoverRegionId) {
        if (IsKeyPressed(KEY_A)) {
            logAdjacencySuggestion(true, debugInspectRegionId, debugHoverRegionId);
        }
        if (IsKeyPressed(KEY_R)) {
            logAdjacencySuggestion(false, debugInspectRegionId, debugHoverRegionId);
        }
    }

    if (debugInspectRegionId >= 0 && debugInspectRegionId != previousInspectRegionId) {
        centerCameraOnRegion(debugInspectRegionId);
    }
}

void ColoringScreen::centerCameraOnRegion(int regionId) {
    const Region* region = mandala->getRegionById(regionId);
    if (region == nullptr) {
        return;
    }

    camera.target = region->getCentroid();
}

void ColoringScreen::drawAnalysisOverlay() const {
    if (!analysisMode) {
        return;
    }

    if (analysisInspectRegionId >= 0) {
        const Region* selectedRegion = mandala->getRegionById(analysisInspectRegionId);
        if (selectedRegion != nullptr) {
            selectedRegion->drawWithColor(Fade(Colors::Cyan, 0.45f), Colors::DarkCyan, 6.0f);

            const auto& neighbors = mandala->getAdjacencyGraph().getAdjacentRegions(analysisInspectRegionId);
            for (int neighborId : neighbors) {
                const Region* neighborRegion = mandala->getRegionById(neighborId);
                if (neighborRegion != nullptr) {
                    neighborRegion->drawWithColor(Fade(Colors::Blue, 0.30f), Colors::Blue, 3.0f);
                }
            }
        }
    }

    if (analysisHoverRegionId >= 0 && analysisHoverRegionId != analysisInspectRegionId) {
        const Region* hoverRegion = mandala->getRegionById(analysisHoverRegionId);
        if (hoverRegion != nullptr) {
            hoverRegion->drawWithColor(Fade(Colors::LightSkyBlue, 0.35f), Colors::DodgerBlue, 2.0f);
        }
    }
}

void ColoringScreen::drawDebugOverlay() const {
    if (!debugAdjacencyMode) {
        return;
    }

    if (debugInspectRegionId < 0) {
        return;
    }

    const Region* selectedRegion = mandala->getRegionById(debugInspectRegionId);
    if (selectedRegion == nullptr) {
        return;
    }

    selectedRegion->drawWithColor(Fade(Colors::Cyan, 0.45f), Colors::DarkCyan, 6.0f);

    const auto& neighbors = mandala->getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
    for (int neighborId : neighbors) {
        const Region* neighborRegion = mandala->getRegionById(neighborId);
        if (neighborRegion == nullptr) {
            continue;
        }

        neighborRegion->drawWithColor(Fade(Colors::Blue, 0.30f), Colors::Blue, 4.0f);
    }

    if (debugHoverRegionId >= 0 && debugHoverRegionId != debugInspectRegionId) {
        const Region* hoverRegion = mandala->getRegionById(debugHoverRegionId);
        if (hoverRegion != nullptr) {
            hoverRegion->drawWithColor(Fade(Colors::LightSkyBlue, 0.35f), Colors::DodgerBlue, 2.0f);
        }
    }
}

int ColoringScreen::getRegionIdAtWorldPosition(Vector2 worldPos) const {
    const auto& regions = mandala->getRegions();
    for (const auto& region : regions) {
        if (region.isPointInRegion(worldPos) && region.isColorable()) {
            return region.getId();
        }
    }

    return -1;
}

bool ColoringScreen::isPointerOverUi(Vector2 screenPos) const {
    if (CheckCollisionPointRec(screenPos, backButton.getBounds())) {
        return true;
    }

    if (CheckCollisionPointRec(screenPos, analysisMode ? analysisCloseButton.getBounds() : analysisButton.getBounds())) {
        return true;
    }

    if (analysisMode && CheckCollisionPointRec(screenPos, analysisClearButton.getBounds())) {
        return true;
    }

    for (const auto& button : colorButtons) {
        if (CheckCollisionPointRec(screenPos, button.getBounds())) {
            return true;
        }
    }

    return false;
}

void ColoringScreen::logAdjacencySuggestion(bool shouldExist, int regionA, int regionB) {
    int a = std::min(regionA, regionB);
    int b = std::max(regionA, regionB);
    std::pair<int, int> pair = {a, b};

    bool currentlyAdjacent = mandala->getAdjacencyGraph().areAdjacent(a, b);

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

void ColoringScreen::layoutTopButtons() {
    float rightMargin = 20.0f;
    float mainButtonWidth = 190.0f;

    float mainButtonX = GetScreenWidth() - mainButtonWidth - rightMargin;
    analysisButton.setPosition(mainButtonX, 20.0f);
    analysisCloseButton.setPosition(mainButtonX, 20.0f);

    float controlsY = 80.0f;
    analysisClearButton.setPosition(GetScreenWidth() - 70.0f - rightMargin, controlsY);
}
