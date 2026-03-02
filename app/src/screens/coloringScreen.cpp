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
        : mandala(mandala), colorPalette(), colorButtons(), 
            backButton(0, 0, 1, 1, "BACK"),
            undoButton(0, 0, 1, 1, "UNDO"),
            validateButton(0, 0, 1, 1, "VALIDATE"),
            analysisButton(0, 0, 1, 1, "ANALYSIS"),
            analysisCloseButton(0, 0, 1, 1, "EXIT ANALYSIS"),
            analysisClearButton(0, 0, 1, 1, "CLEAR"),
            winImageSaved(false), gameWon(false), returnRequested(false),
            pendingColorChangesForSave(0), saveRequested(false),
            cameraInputManager(MIN_ZOOM, MAX_ZOOM, ZOOM_STEP, TOUCH_PAN_START_THRESHOLD, TOUCH_PINCH_SENSITIVITY),
            camera{}, zoom(1.0f) {

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

    layoutTopButtons();
}

void ColoringScreen::update(float deltaTime) {
    layoutTopButtons();
    cameraInputManager.update(camera, zoom, [this](Vector2 pointerPos) {
        return isPointerOverUi(pointerPos);
    });

    if (interactionManager.updateBackNavigation(backButton)) {
        returnRequested = true;
        saveRequested = true;
        return;
    }

    undoButton.update();
    if (undoButton.isClicked()) {
        actionManager.undoLast(*mandala);
    }

    validateButton.update();
    if (validateButton.isClicked()) {
        inspector.validateAdjacency(*mandala);
    }  

    interactionManager.updateColorButtons(colorButtons, colorPalette);

    updateAnalysisInteractions();
    updateDebugInteractions();
    int regionId = interactionManager.getRegionIdForColorSelection(
        *mandala,
        camera,
        isPointerOverUi(Input::GetPointerPosition()),
        cameraInputManager.isDraggingCamera(),
        inspector.isAnalysisMode());

    if (regionId >= 0 && actionManager.applyColorChange(*mandala, regionId, colorPalette.getSelectedColorIndex())) {
        pendingColorChangesForSave++;
        if (pendingColorChangesForSave >= 5) {
            saveRequested = true;
            pendingColorChangesForSave = 0;
        }
    }

    if (mandala->isValidColoring()) {
        gameWon = true;
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
    mandala->draw(colorPalette.getColors(), false);
    inspector.drawValidationOverlay(*mandala);
    drawAnalysisOverlay();
    drawDebugOverlay();
    EndMode2D();

    inspector.drawDebugInfoPanel(*mandala, uiScale);

    drawColorPalette();
    backButton.draw();
    undoButton.draw();
    validateButton.draw();
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

bool ColoringScreen::consumeSaveRequested() {
    if (!saveRequested) {
        return false;
    }

    saveRequested = false;
    pendingColorChangesForSave = 0;
    return true;
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

void ColoringScreen::updateAnalysisInteractions() {
    interactionManager.updateAnalysisControls(
        inspector,
        *mandala,
        camera,
        cameraInputManager.isDraggingCamera(),
    backButton,
    undoButton,
    validateButton,
    analysisButton,
    analysisCloseButton,
    analysisClearButton,
    colorButtons);
}

void ColoringScreen::updateDebugInteractions() {
    interactionManager.updateDebugControls(inspector, *mandala, camera, camera);
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
    inspector.drawAnalysisOverlay(*mandala, colorPalette.getColors());
}

void ColoringScreen::drawDebugOverlay() const {
    inspector.drawDebugOverlay(*mandala);
}

bool ColoringScreen::isPointerOverUi(Vector2 screenPos) const {
    return interactionManager.isPointerOverUi(
        screenPos,
        inspector.isAnalysisMode(),
        backButton,
        undoButton,
        validateButton,
        analysisButton,
        analysisCloseButton,
        analysisClearButton,
        colorButtons);
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

    float undoButtonWidth = mobileLayout ? 126.0f * uiScale : 110.0f;
    float undoButtonHeight = topButtonHeight;
    float undoButtonX = GetScreenWidth() - undoButtonWidth - rightMargin;
    float undoButtonY = GetScreenHeight() - undoButtonHeight - (20.0f * uiScale);
    undoButton.setPosition(undoButtonX, undoButtonY);
    undoButton.setSize(undoButtonWidth, undoButtonHeight);

    float validateButtonX = GetScreenWidth() - 2*mainButtonWidth - rightMargin;
    validateButton.setPosition(validateButtonX, topMargin);
    validateButton.setSize(mainButtonWidth, topButtonHeight);
    
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