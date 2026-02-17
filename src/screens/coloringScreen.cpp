#include "coloringScreen.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include "raymath.h"
#include <sstream>

namespace {
    constexpr float SCREEN_CENTER_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
    constexpr float MIN_ZOOM = 0.02f;
    constexpr float MAX_ZOOM = 4.0f;
    constexpr float ZOOM_STEP = 0.01f;
}

ColoringScreen::ColoringScreen(std::shared_ptr<Mandala> mandala)
        : mandala(mandala), colorPalette(), colorButtons(), backButton(20, 20, 100, 50, "BACK"),
            gameWon(false), returnRequested(false), camera{}, zoom(1.0f),
            debugAdjacencyMode(false), debugInspectRegionId(-1), debugHoverRegionId(-1) {

        camera.target = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
        camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
        camera.rotation = 0.0f;
        camera.zoom = zoom;
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        float x = 50 + i * 130;
        float y = 550;
        colorButtons.emplace_back(x, y, 100, 40, "");
    }
}

void ColoringScreen::update(float deltaTime) {
    updateZoom();

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

    updateDebugOverlay();
    handleColorSelection();

    if (mandala->isValidColoring()) {
        gameWon = true;
    }
}

void ColoringScreen::handleColorSelection() {
    if (Input::IsPointerPressed()) {
        Vector2 pointerPos = Input::GetPointerPosition();
        Vector2 worldPos = GetScreenToWorld2D(pointerPos, camera);
        Region* region = mandala->getRegionAtPoint(worldPos);
        if (region != nullptr) {
            region->setColor(colorPalette.getSelectedColorIndex());
        }
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
        zoom = Clamp(zoom + zoomDelta, MIN_ZOOM, MAX_ZOOM);
        camera.zoom = zoom;
    }
}

void ColoringScreen::draw() {
    ClearBackground(Colors::LightBlue);
    
    DrawText(mandala->getName().c_str(), 150, 20, 30, Colors::Black);

    BeginMode2D(camera);
    mandala->draw(colorPalette.getColors());
    drawDebugOverlay();
    EndMode2D();

    if (debugAdjacencyMode) {
        DrawRectangle(15, 70, 760, 58, Fade(Colors::White, 0.85f));
        DrawRectangleLines(15, 70, 760, 58, Colors::DarkGray);

        std::ostringstream info;
        info << "DEBUG ADJ: ON  |  Hover: " << debugHoverRegionId
             << "  |  Inspect (Right Click): " << debugInspectRegionId
             << "  |  Clear Inspect: C";
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

void ColoringScreen::updateDebugOverlay() {
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

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    debugHoverRegionId = getRegionIdAtWorldPosition(worldPos);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && debugHoverRegionId >= 0) {
        debugInspectRegionId = debugHoverRegionId;
    }

    if (IsKeyPressed(KEY_C)) {
        debugInspectRegionId = -1;
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

    selectedRegion->drawWithColor(Fade(Colors::Orange, 0.55f), Colors::Orange, 6.0f);

    const auto& neighbors = mandala->getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
    for (int neighborId : neighbors) {
        const Region* neighborRegion = mandala->getRegionById(neighborId);
        if (neighborRegion == nullptr) {
            continue;
        }

        neighborRegion->drawWithColor(Fade(Colors::Purple, 0.45f), Colors::Purple, 4.0f);
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
