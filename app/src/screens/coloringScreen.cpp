#include "coloringScreen.h"
#include "../ui/colors.h"
#include "../ui/colorTileRenderer.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>
#include <utility>
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

    int getAvailableEditableColorCount(const Mandala& mandala, int totalCount) {
        const int editableCount = std::max(0, totalCount - ColorPalette::LockedColorSlots);

        const int minimumRequired = mandala.getMinimumColorCount();
        if (minimumRequired > 0) {
            return std::min(editableCount, minimumRequired);
        }

        if (mandala.isHardMode()) {
            return editableCount;
        }

        return std::min(editableCount, ColorPalette::NormalEditableColorCount);
    }
}

ColoringScreen::ColoringScreen(std::shared_ptr<Mandala> mandala,
                               const std::vector<Color>& customPaletteColors,
                               bool readOnlyMode)
        : mandala(mandala), colorPalette(), colorButtons(), 
            backButton(0, 0, 1, 1, "BACK"),
            undoButton(0, 0, 1, 1, "UNDO"),
            validateButton(0, 0, 1, 1, "VALIDATE"),
            analysisButton(0, 0, 1, 1, "ANALYSIS"),
            analysisCloseButton(0, 0, 1, 1, "EXIT ANALYSIS"),
            analysisClearButton(0, 0, 1, 1, "CLEAR"),
            gameWon(false), returnRequested(false),
            inspector(),
            cameraInputManager(MIN_ZOOM, MAX_ZOOM, ZOOM_STEP, TOUCH_PAN_START_THRESHOLD, TOUCH_PINCH_SENSITIVITY),
            actionManager(),
            interactionManager(),
            camera{}, zoom(1.0f),
            pendingColorChangesForSave(0), saveRequested(false), readOnlyMode(readOnlyMode) {

        camera.target = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
        camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
        camera.rotation = 0.0f;
        camera.zoom = zoom;
        fitCameraToMandala();

    if (!customPaletteColors.empty()) {
        colorPalette.setColors(customPaletteColors);
    }

    int availableEditableColorCount = getAvailableEditableColorCount(*mandala, colorPalette.getColorCount());
    const int maxAllowedColorIndex = availableEditableColorCount;
    if (availableEditableColorCount > 0) {
        if (colorPalette.getSelectedColorIndex() < 0 || colorPalette.getSelectedColorIndex() > maxAllowedColorIndex) {
            colorPalette.setSelectedColorIndex(0);
        }
    }
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        float x = 50 + i * 130;
        float y = 550;
        colorButtons.emplace_back(x, y, 100, 40, "");
    }

    layoutTopButtons();
}

void ColoringScreen::update(float deltaTime) {
    (void)deltaTime;
    layoutTopButtons();
    cameraInputManager.update(camera, zoom, [this](Vector2 pointerPos) {
        return isPointerOverUi(pointerPos);
    });

    if (interactionManager.updateBackNavigation(backButton)) {
        returnRequested = true;
        saveRequested = !readOnlyMode;
        return;
    }

    if (readOnlyMode) {
        updateAnalysisInteractions();
        updateDebugInteractions();
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

    int availableEditableColorCount = getAvailableEditableColorCount(*mandala, static_cast<int>(colorButtons.size()));
    const int maxAllowedColorIndex = availableEditableColorCount;
    for (int colorIndex = 0; colorIndex <= maxAllowedColorIndex; colorIndex++) {
        colorButtons[colorIndex].update();
        if (colorButtons[colorIndex].isClicked()) {
            colorPalette.setSelectedColorIndex(colorIndex);
        }
    }

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

    float statusBadgeX = static_cast<float>(titleX + MeasureText(titleText.c_str(), titleFont) + static_cast<int>(10.0f * uiScale));
    float statusBadgeY = static_cast<float>(titleY + static_cast<int>(6.0f * uiScale));

    if (mandala->isHardMode()) {
        float hardBadgeWidth = 72.0f * uiScale;
        float hardBadgeHeight = 24.0f * uiScale;
        DrawRectangleRec({statusBadgeX, statusBadgeY, hardBadgeWidth, hardBadgeHeight}, Colors::Crimson);
        DrawRectangleLinesEx({statusBadgeX, statusBadgeY, hardBadgeWidth, hardBadgeHeight}, 2.0f, Colors::Black);

        int hardTextSize = std::max(12, static_cast<int>(12.0f * uiScale));
        const char* hardLabel = "HARD";
        int hardTextWidth = MeasureText(hardLabel, hardTextSize);
        int hardTextX = static_cast<int>(statusBadgeX + (hardBadgeWidth - hardTextWidth) * 0.5f);
        int hardTextY = static_cast<int>(statusBadgeY + (hardBadgeHeight - hardTextSize) * 0.5f);
        DrawText(hardLabel, hardTextX, hardTextY, hardTextSize, Colors::White);

        statusBadgeX += hardBadgeWidth + (8.0f * uiScale);

        if (mandala->getMinimumColorCount() > 0) {
            const std::string minLabel = "MIN " + std::to_string(mandala->getMinimumColorCount());
            int minTextSize = std::max(12, static_cast<int>(12.0f * uiScale));
            float minBadgeWidth = static_cast<float>(MeasureText(minLabel.c_str(), minTextSize)) + (18.0f * uiScale);
            float minBadgeHeight = 24.0f * uiScale;

            DrawRectangleRec({statusBadgeX, statusBadgeY, minBadgeWidth, minBadgeHeight}, Colors::DarkBlue);
            DrawRectangleLinesEx({statusBadgeX, statusBadgeY, minBadgeWidth, minBadgeHeight}, 2.0f, Colors::Black);

            int minTextX = static_cast<int>(statusBadgeX + (minBadgeWidth - MeasureText(minLabel.c_str(), minTextSize)) * 0.5f);
            int minTextY = static_cast<int>(statusBadgeY + (minBadgeHeight - minTextSize) * 0.5f);
            DrawText(minLabel.c_str(), minTextX, minTextY, minTextSize, Colors::White);

            statusBadgeX += minBadgeWidth + (8.0f * uiScale);
        }
    }

    if (readOnlyMode) {
        float badgeWidth = 72.0f * uiScale;
        float badgeHeight = 24.0f * uiScale;
        float badgeX = statusBadgeX;
        float badgeY = statusBadgeY;

        DrawRectangleRec({badgeX, badgeY, badgeWidth, badgeHeight}, Colors::Gold);
        DrawRectangleLinesEx({badgeX, badgeY, badgeWidth, badgeHeight}, 2.0f, Colors::Black);

        int doneTextSize = std::max(12, static_cast<int>(12.0f * uiScale));
        const char* doneLabel = "DONE";
        int doneTextWidth = MeasureText(doneLabel, doneTextSize);
        int doneTextX = static_cast<int>(badgeX + (badgeWidth - doneTextWidth) * 0.5f);
        int doneTextY = static_cast<int>(badgeY + (badgeHeight - doneTextSize) * 0.5f);
        DrawText(doneLabel, doneTextX, doneTextY, doneTextSize, Colors::Black);
    }

    BeginMode2D(camera);
    mandala->draw(colorPalette.getColors(), false);
    inspector.drawValidationOverlay(*mandala, camera.zoom);
    drawAnalysisOverlay();
    drawDebugOverlay();
    EndMode2D();

    inspector.drawDebugInfoPanel(*mandala, uiScale);

    if (!readOnlyMode) {
        drawColorPalette();
    }
    backButton.draw();
    if (!readOnlyMode) {
        undoButton.draw();
    }
    if (!readOnlyMode) {
        validateButton.draw();
    }
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

    int availableEditableColorCount = getAvailableEditableColorCount(*mandala, colorPalette.getColorCount());
    if (availableEditableColorCount <= 0) {
        return;
    }

    float uiScale = getUiScale();
    Rectangle firstButton = colorButtons[0].getBounds();
    int labelSize = static_cast<int>(16.0f * uiScale);
    int labelX = static_cast<int>(firstButton.x);
    int labelY = static_cast<int>(firstButton.y - labelSize - (10.0f * uiScale));
    DrawText("Colors:", labelX, labelY, labelSize, Colors::Black);
    
    for (int colorIndex = 0; colorIndex <= availableEditableColorCount; colorIndex++) {
        Color color = colorPalette.getColor(colorIndex);
        Rectangle bounds = colorButtons[colorIndex].getBounds();
        ColorTileRenderer::drawColorTile(color, bounds, uiScale);
        
        if (colorIndex == colorPalette.getSelectedColorIndex()) {
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
    if (readOnlyMode) {
        undoButton.setPosition(0.0f, 0.0f);
        undoButton.setSize(0.0f, 0.0f);
    } else {
        float undoButtonX = GetScreenWidth() - undoButtonWidth - rightMargin;
        float undoButtonY = GetScreenHeight() - undoButtonHeight - (20.0f * uiScale);
        undoButton.setPosition(undoButtonX, undoButtonY);
        undoButton.setSize(undoButtonWidth, undoButtonHeight);
    }

    float validateButtonX = GetScreenWidth() - 2*mainButtonWidth - rightMargin;
    if (readOnlyMode) {
        validateButton.setPosition(0.0f, 0.0f);
        validateButton.setSize(0.0f, 0.0f);
    } else {
        validateButton.setPosition(validateButtonX, topMargin);
        validateButton.setSize(mainButtonWidth, topButtonHeight);
    }
    
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
    int availableEditableColorCount = getAvailableEditableColorCount(*mandala, colorCount);

    if (readOnlyMode) {
        for (int i = 0; i < colorCount; i++) {
            colorButtons[i].setPosition(0.0f, 0.0f);
            colorButtons[i].setSize(0.0f, 0.0f);
        }
        return;
    }

    const bool useTwoColumns = availableEditableColorCount >= 8;

    float paletteX = leftMargin;
    float paletteTop = controlsY;
    float paletteBottomMargin = 20.0f * uiScale;
    float availableHeight = std::max(1.0f, static_cast<float>(GetScreenHeight()) - paletteTop - paletteBottomMargin);
    float verticalGap = mobileLayout ? (8.0f * uiScale) : 10.0f;
    int columns = useTwoColumns ? 2 : 1;
    int visibleColorCount = availableEditableColorCount + 1;
    int rows = std::max(1, (visibleColorCount + columns - 1) / columns);
    float buttonWidth = mobileLayout ? (88.0f * uiScale) : 90.0f;
    float horizontalGap = std::max(10.0f * uiScale, 12.0f);

    if (useTwoColumns) {
        float maxWidthForTwoCols = (GetScreenWidth() * 0.42f) - leftMargin;
        float twoColWidth = (maxWidthForTwoCols - horizontalGap) * 0.5f;
        buttonWidth = Clamp(twoColWidth, 64.0f * uiScale, buttonWidth);
    }

    float buttonHeightBySpace = (availableHeight - ((rows - 1) * verticalGap)) / std::max(1, rows);
    float minButtonHeight = mobileLayout ? (24.0f * uiScale) : 36.0f;
    float preferredButtonHeight = mobileLayout ? (36.0f * uiScale) : 48.0f;
    float buttonHeight = Clamp(buttonHeightBySpace, minButtonHeight, preferredButtonHeight);

    for (int i = 0; i < colorCount; i++) {
        if (i > availableEditableColorCount) {
            colorButtons[i].setPosition(0.0f, 0.0f);
            colorButtons[i].setSize(0.0f, 0.0f);
            continue;
        }

        int visibleIndex = i;
        int row = visibleIndex / columns;
        int col = visibleIndex % columns;
        float buttonX = paletteX + col * (buttonWidth + horizontalGap);
        float buttonY = paletteTop + row * (buttonHeight + verticalGap);
        colorButtons[i].setPosition(buttonX, buttonY);
        colorButtons[i].setSize(buttonWidth, buttonHeight);
    }
}