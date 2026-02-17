#include "coloringScreen.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include "raymath.h"

namespace {
    constexpr float SCREEN_CENTER_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
    constexpr float MIN_ZOOM = 0.02f;
    constexpr float MAX_ZOOM = 4.0f;
    constexpr float ZOOM_STEP = 0.01f;
}

ColoringScreen::ColoringScreen(std::shared_ptr<Mandala> mandala)
        : mandala(mandala), colorPalette(), colorButtons(), backButton(20, 20, 100, 50, "BACK"),
            gameWon(false), returnRequested(false), camera{}, zoom(1.0f) {

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
    EndMode2D();

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
