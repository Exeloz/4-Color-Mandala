#include "coloringScreen.h"
#include "../ui/colors.h"
#include "../ui/colorTileRenderer.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>
#include <utility>
#include <sstream>
#include <ctime>
#include <cfloat>

namespace {
    constexpr float SCREEN_CENTER_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
    constexpr float MIN_ZOOM = 0.02f;
    constexpr float MAX_ZOOM = 4.0f;
    constexpr float ZOOM_STEP = 0.01f;
    constexpr float TOUCH_PAN_START_THRESHOLD = 8.0f;
    constexpr float TOUCH_PINCH_SENSITIVITY = 1.2f;
    constexpr float CAMERA_FIT_MARGIN = 24.0f;

    bool isMobileLayout() {
        return true;
    }

    float getUiScale() {
        float widthScale = static_cast<float>(GetScreenWidth()) / 860.0f;
        float heightScale = static_cast<float>(GetScreenHeight()) / 420.0f;
        return Clamp(std::min(widthScale, heightScale), 0.75f, 2.4f);
    }

    std::string fitTextWithEllipsis(const std::string& text, int textSize, int maxWidth) {
        const std::string ellipsis = "...";
        if (MeasureText(text.c_str(), textSize) <= maxWidth) {
            return text;
        }

        std::string trimmed = text;
        while (!trimmed.empty()) {
            trimmed.pop_back();
            std::string candidate = trimmed + ellipsis;
            if (MeasureText(candidate.c_str(), textSize) <= maxWidth) {
                return candidate;
            }
        }

        return ellipsis;
    }
}

ColoringScreen::ColoringScreen(std::shared_ptr<Mandala> mandala, const std::vector<Color>& customPaletteColors)
        : mandala(mandala), colorPalette(), colorButtons(), backButton(20, 20, 100, 50, "BACK"),
            analysisButton(590, 20, 190, 50, "ANALYSIS"),
            analysisCloseButton(590, 20, 190, 50, "EXIT ANALYSIS"),
            analysisClearButton(710, 80, 70, 44, "CLEAR"),
            winImageSaved(false), gameWon(false), returnRequested(false), camera{}, zoom(1.0f),
            isPanning(false), isDraggingCamera(false), isPinching(false), lastPinchDistance(0.0f), lastPanPointer{} {

        camera.target = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
        camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
        camera.rotation = 0.0f;
        camera.zoom = zoom;
        fitCameraToMandala();

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
    if (!inspector.isAnalysisMode()) {
        handleColorSelection();
    }

    if (mandala->isValidColoring()) {
        gameWon = true;
    }
}

void ColoringScreen::handleColorSelection() {
    if (!isDraggingCamera && Input::IsPointerPressed()) {
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

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    int touchCount = GetTouchPointCount();
    if (touchCount >= 2) {
        shouldPan = false;
    } else if (touchCount == 1) {
        pointerPos = GetTouchPosition(0);
        shouldPan = !isPointerOverUi(pointerPos);
    } else {
        shouldPan = false;
    }
#endif

    if (shouldPan) {
        if (!isPanning) {
            isPanning = true;
            isDraggingCamera = false;
            lastPanPointer = pointerPos;
            return;
        }

        Vector2 delta = Vector2Subtract(pointerPos, lastPanPointer);
        float deltaLength = Vector2Length(delta);

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
        if (isDraggingCamera || deltaLength >= TOUCH_PAN_START_THRESHOLD) {
            camera.target = Vector2Subtract(camera.target, Vector2Scale(delta, 1.0f / camera.zoom));
            isDraggingCamera = true;
        }
#else
        if (deltaLength > 0.0f) {
            camera.target = Vector2Subtract(camera.target, Vector2Scale(delta, 1.0f / camera.zoom));
            isDraggingCamera = true;
        }
#endif

        lastPanPointer = pointerPos;
    } else {
        isPanning = false;
        isDraggingCamera = false;
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
    int touchCount = GetTouchPointCount();
    if (touchCount >= 2) {
        Vector2 touchA = GetTouchPosition(0);
        Vector2 touchB = GetTouchPosition(1);
        float currentPinchDistance = Vector2Distance(touchA, touchB);

        if (isPinching) {
            Vector2 screenSize = {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
            float screenDiagonal = Vector2Length(screenSize);
            float normalizedPinchDelta = (currentPinchDistance - lastPinchDistance) / std::max(1.0f, screenDiagonal);
            zoomDelta += normalizedPinchDelta * TOUCH_PINCH_SENSITIVITY;
        }

        lastPinchDistance = currentPinchDistance;
        isPinching = true;
    } else {
        isPinching = false;
        lastPinchDistance = 0.0f;
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

    float uiScale = getUiScale();
    Rectangle backBounds = backButton.getBounds();
    int titleFont = static_cast<int>(30.0f * uiScale);
    int titleX = static_cast<int>(backBounds.x + backBounds.width + (16.0f * uiScale));
    int titleY = static_cast<int>(20.0f * uiScale);

    int titleMaxWidth = GetScreenWidth() - titleX - static_cast<int>(20.0f * uiScale);
    Rectangle actionBounds = inspector.isAnalysisMode() ? analysisCloseButton.getBounds() : analysisButton.getBounds();
    titleMaxWidth = static_cast<int>(actionBounds.x - titleX - (12.0f * uiScale));
    titleMaxWidth = std::max(titleMaxWidth, static_cast<int>(140.0f * uiScale));
    std::string titleText = fitTextWithEllipsis(mandala->getName(), titleFont, titleMaxWidth);
    DrawText(titleText.c_str(), titleX, titleY, titleFont, Colors::Black);

    BeginMode2D(camera);
    mandala->draw(colorPalette.getColors(), inspector.isAnalysisMode());
    drawAnalysisOverlay();
    drawDebugOverlay();
    EndMode2D();

    inspector.drawDebugInfoPanel(*mandala, uiScale);

    drawColorPalette();
    backButton.draw();
    if (inspector.isAnalysisMode()) {
        analysisCloseButton.draw();
        analysisClearButton.draw();
    } else {
        analysisButton.draw();
    }
}

void ColoringScreen::drawColorPalette() {
    if (colorButtons.empty()) {
        return;
    }

    float uiScale = getUiScale();
    Rectangle firstButton = colorButtons.front().getBounds();
    int labelSize = static_cast<int>(16.0f * uiScale);
    int labelX = static_cast<int>(firstButton.x);
    int labelY = static_cast<int>(firstButton.y - labelSize - (10.0f * uiScale));
    DrawText("Colors:", labelX, labelY, labelSize, Colors::Black);
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        Color color = colorPalette.getColor(i);
        Rectangle bounds = colorButtons[i].getBounds();
        ColorTileRenderer::drawColorTile(color, bounds, uiScale);
        
        if (i == colorPalette.getSelectedColorIndex()) {
            DrawRectangleLinesEx(bounds, 7.0f, Colors::Black);
        } else {
            DrawRectangleLinesEx(bounds, 2.0f, Colors::Gray);
        }
    }
}

bool ColoringScreen::isGameWon() const {
    return gameWon;
}

bool ColoringScreen::shouldReturnToSelection() const {
    return returnRequested;
}

void ColoringScreen::saveWinImage() {
    if (winImageSaved) {
        return;
    }

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    RenderTexture2D renderTexture = LoadRenderTexture(screenWidth, screenHeight);

    BeginTextureMode(renderTexture);
    ClearBackground(Colors::White);
    BeginMode2D(camera);
    mandala->draw(colorPalette.getColors());
    EndMode2D();
    EndTextureMode();

    Image image = LoadImageFromTexture(renderTexture.texture);
    ImageFlipVertical(&image);

    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    char timestamp[32] = {0};
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localTime);

    std::string fileName = "mandala_win_" + std::string(timestamp) + ".png";

#if defined(PLATFORM_ANDROID)
    const std::string downloadPath1 = "/storage/emulated/0/Download/" + fileName;
    const std::string downloadPath2 = "/sdcard/Download/" + fileName;

    bool exportOk = ExportImage(image, downloadPath1.c_str());
    std::string exportedPath = downloadPath1;

    if (!exportOk) {
        exportOk = ExportImage(image, downloadPath2.c_str());
        exportedPath = downloadPath2;
    }

    if (!exportOk) {
        exportOk = ExportImage(image, fileName.c_str());
        exportedPath = fileName;
    }

    if (exportOk) {
        TraceLog(LOG_INFO, "[WIN] Mandala image exported: %s", exportedPath.c_str());
    } else {
        TraceLog(LOG_WARNING, "[WIN] Failed to export mandala image.");
    }
#else
    bool exportOk = ExportImage(image, fileName.c_str());
    if (exportOk) {
        TraceLog(LOG_INFO, "[WIN] Mandala image exported: %s", fileName.c_str());
    } else {
        TraceLog(LOG_WARNING, "[WIN] Failed to export mandala image.");
    }
#endif

    UnloadImage(image);
    UnloadRenderTexture(renderTexture);
    winImageSaved = true;
}

void ColoringScreen::updateAnalysisOverlay() {
    if (!inspector.isAnalysisMode()) {
        analysisButton.update();
        if (analysisButton.isClicked()) {
            inspector.enterAnalysisMode();
        }
        return;
    }

    analysisCloseButton.update();
    analysisClearButton.update();

    if (analysisCloseButton.isClicked()) {
        inspector.exitAnalysisMode();
        return;
    }

    if (analysisClearButton.isClicked()) {
        inspector.clearAnalysisSelection();
        return;
    }

    Vector2 pointerPos = Input::GetPointerPosition();
    inspector.updateAnalysis(*mandala, camera, isPointerOverUi(pointerPos), isDraggingCamera);
}

void ColoringScreen::updateDebugOverlay() {
    inspector.updateDebug(*mandala, camera, camera);
}

void ColoringScreen::fitCameraToMandala() {
    const auto& regions = mandala->getRegions();
    if (regions.empty()) {
        return;
    }

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;

    bool hasVertex = false;
    for (const auto& region : regions) {
        const auto& vertices = region.getVertices();
        for (const auto& vertex : vertices) {
            minX = std::min(minX, vertex.x);
            minY = std::min(minY, vertex.y);
            maxX = std::max(maxX, vertex.x);
            maxY = std::max(maxY, vertex.y);
            hasVertex = true;
        }
    }

    if (!hasVertex) {
        return;
    }

    float contentWidth = std::max(maxX - minX, 1.0f);
    float contentHeight = std::max(maxY - minY, 1.0f);

    float availableWidth = std::max(1.0f, static_cast<float>(GetScreenWidth()) - (2.0f * CAMERA_FIT_MARGIN));
    float availableHeight = std::max(1.0f, static_cast<float>(GetScreenHeight()) - (2.0f * CAMERA_FIT_MARGIN));

    float fitZoomX = availableWidth / contentWidth;
    float fitZoomY = availableHeight / contentHeight;

    zoom = Clamp(std::min(fitZoomX, fitZoomY), MIN_ZOOM, MAX_ZOOM);
    camera.zoom = zoom;
    camera.target = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f};
}

void ColoringScreen::drawAnalysisOverlay() const {
    inspector.drawAnalysisOverlay(*mandala);
}

void ColoringScreen::drawDebugOverlay() const {
    inspector.drawDebugOverlay(*mandala);
}

bool ColoringScreen::isPointerOverUi(Vector2 screenPos) const {
    if (CheckCollisionPointRec(screenPos, backButton.getBounds())) {
        return true;
    }

    if (CheckCollisionPointRec(screenPos, inspector.isAnalysisMode() ? analysisCloseButton.getBounds() : analysisButton.getBounds())) {
        return true;
    }

    if (inspector.isAnalysisMode() && CheckCollisionPointRec(screenPos, analysisClearButton.getBounds())) {
        return true;
    }

    for (const auto& button : colorButtons) {
        if (CheckCollisionPointRec(screenPos, button.getBounds())) {
            return true;
        }
    }

    return false;
}
void ColoringScreen::layoutTopButtons() {
    float uiScale = getUiScale();
    bool mobileLayout = isMobileLayout();

    float leftMargin = 20.0f * uiScale;
    float rightMargin = 20.0f * uiScale;
    float topMargin = 20.0f * uiScale;

    float topButtonHeight = mobileLayout ? 56.0f * uiScale : 50.0f;
    float backButtonWidth = mobileLayout ? 136.0f * uiScale : 100.0f;
    float mainButtonWidth = mobileLayout ? 190.0f * uiScale : 190.0f;
    float clearButtonWidth = mobileLayout ? 86.0f * uiScale : 70.0f;
    float clearButtonHeight = mobileLayout ? 44.0f * uiScale : 44.0f;

    backButton.setPosition(leftMargin, topMargin);
    backButton.setSize(backButtonWidth, topButtonHeight);

    float controlsY = topMargin + topButtonHeight + (10.0f * uiScale);

    float mainButtonX = GetScreenWidth() - mainButtonWidth - rightMargin;
    analysisButton.setPosition(mainButtonX, topMargin);
    analysisButton.setSize(mainButtonWidth, topButtonHeight);
    analysisCloseButton.setPosition(mainButtonX, topMargin);
    analysisCloseButton.setSize(mainButtonWidth, topButtonHeight);

    analysisClearButton.setPosition(GetScreenWidth() - clearButtonWidth - rightMargin, controlsY);
    analysisClearButton.setSize(clearButtonWidth, clearButtonHeight);

    if (colorButtons.empty()) {
        return;
    }

    int colorCount = static_cast<int>(colorButtons.size());

    float paletteX = leftMargin;
    float paletteTop = controlsY;
    float paletteBottomMargin = 20.0f * uiScale;
    float availableHeight = std::max(1.0f, static_cast<float>(GetScreenHeight()) - paletteTop - paletteBottomMargin);
    float verticalGap = mobileLayout ? (8.0f * uiScale) : 10.0f;
    int rows = colorCount;
    float buttonWidth = mobileLayout ? (88.0f * uiScale) : 90.0f;

    float buttonHeightBySpace = (availableHeight - ((rows - 1) * verticalGap)) / std::max(1, rows);
    float minButtonHeight = mobileLayout ? (24.0f * uiScale) : 36.0f;
    float preferredButtonHeight = mobileLayout ? (36.0f * uiScale) : 48.0f;
    float buttonHeight = Clamp(buttonHeightBySpace, minButtonHeight, preferredButtonHeight);

    for (int i = 0; i < colorCount; i++) {
        int row = i;
        float buttonX = paletteX;
        float buttonY = paletteTop + row * (buttonHeight + verticalGap);
        colorButtons[i].setPosition(buttonX, buttonY);
        colorButtons[i].setSize(buttonWidth, buttonHeight);
    }
}
