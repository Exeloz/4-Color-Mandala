#include "coloringScreen.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>
#include <utility>

namespace {
    constexpr float SCREEN_CENTER_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
    constexpr float MIN_ZOOM = 0.02f;
    constexpr float MAX_ZOOM = 4.0f;
    constexpr float ZOOM_STEP = 0.01f;
    constexpr float TOUCH_PAN_START_THRESHOLD = 8.0f;
    constexpr float TOUCH_PINCH_SENSITIVITY = 1.2f;

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
                               bool readOnlyMode,
                               std::vector<StatusBadge> ruleBadges)
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
            pendingColorChangesForSave(0), saveRequested(false), readOnlyMode(readOnlyMode),
            ruleBadges(std::move(ruleBadges)), allBadges(), badgePopupIndex(-1), lastColorHintLabelCache() {

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

    auto handleBadgeTap = [this]() {
        if (!Input::IsPointerPressed()) {
            return;
        }
        Vector2 ptr = Input::GetPointerPosition();
        bool hitBadge = false;
        for (int i = 0; i < static_cast<int>(badgeRects.size()); i++) {
            if (CheckCollisionPointRec(ptr, badgeRects[i])) {
                if (!allBadges[i].description.empty()) {
                    badgePopupIndex = (badgePopupIndex == i) ? -1 : i;
                }
                hitBadge = true;
                break;
            }
        }
        if (!hitBadge && badgePopupIndex >= 0) {
            badgePopupIndex = -1;
        }
    };

    if (readOnlyMode) {
        handleBadgeTap();
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

    // Region interaction has priority: only process badge taps if no region was hit.
    if (regionId < 0) {
        handleBadgeTap();
    }

    if (mandala->isValidColoring() && isLastColorHintRuleSatisfied()) {
        gameWon = true;
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