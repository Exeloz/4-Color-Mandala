#include "coloringInteractionManager.h"
#include "../ui/input.h"

bool ColoringInteractionManager::updateBackNavigation(Button& backButton) const {
    backButton.update();
    return backButton.isClicked();
}

void ColoringInteractionManager::updateColorButtons(std::vector<Button>& colorButtons, ColorPalette& colorPalette) const {
    for (int index = 0; index < static_cast<int>(colorButtons.size()); index++) {
        colorButtons[index].update();
        if (colorButtons[index].isClicked()) {
            colorPalette.setSelectedColorIndex(index);
        }
    }
}

void ColoringInteractionManager::updateAnalysisControls(
    ColoringInspector& inspector,
    Mandala& mandala,
    const Camera2D& camera,
    bool isDraggingCamera,
    const Button& backButton,
    const Button& undoButton,
    Button& validateButton,
    Button& analysisButton,
    Button& analysisCloseButton,
    Button& analysisClearButton,
    const std::vector<Button>& colorButtons) const {
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
    bool pointerOverUi = isPointerOverUi(
        pointerPos,
        inspector.isAnalysisMode(),
        backButton,
        undoButton,
        validateButton,
        analysisButton,
        analysisCloseButton,
        analysisClearButton,
        colorButtons);

    inspector.updateAnalysis(mandala, camera, pointerOverUi, isDraggingCamera);
}

void ColoringInteractionManager::updateDebugControls(
    ColoringInspector& inspector,
    Mandala& mandala,
    const Camera2D& camera,
    Camera2D& mutableCamera,
    bool hardModeEnabled) const {
    inspector.updateDebug(mandala, camera, mutableCamera, hardModeEnabled);
}

int ColoringInteractionManager::getRegionIdForColorSelection(
    Mandala& mandala,
    const Camera2D& camera,
    bool pointerOverUi,
    bool isDraggingCamera,
    bool analysisMode) const {
    if (analysisMode || isDraggingCamera || !Input::IsPointerPressed() || pointerOverUi) {
        return -1;
    }

    Vector2 pointerPos = Input::GetPointerPosition();
    Vector2 worldPos = GetScreenToWorld2D(pointerPos, camera);
    Region* region = mandala.getRegionAtPoint(worldPos);
    if (region == nullptr || !region->isColorable()) {
        return -1;
    }

    return region->getId();
}

bool ColoringInteractionManager::isPointerOverUi(
    Vector2 screenPos,
    bool analysisMode,
    const Button& backButton,
    const Button& undoButton,
    const Button& validateButton,
    const Button& analysisButton,
    const Button& analysisCloseButton,
    const Button& analysisClearButton,
    const std::vector<Button>& colorButtons) const {
    if (CheckCollisionPointRec(screenPos, backButton.getBounds())) {
        return true;
    }

    if (CheckCollisionPointRec(screenPos, undoButton.getBounds())) {
        return true;
    }

    if (CheckCollisionPointRec(screenPos, validateButton.getBounds())) {
        return true;
    }

    const Rectangle analysisBounds = analysisMode ? analysisCloseButton.getBounds() : analysisButton.getBounds();
    if (CheckCollisionPointRec(screenPos, analysisBounds)) {
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
