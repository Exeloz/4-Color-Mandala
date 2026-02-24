#pragma once

#include "../mandala/mandala.h"
#include "../ui/button.h"
#include "../ui/colorPalette.h"
#include "coloringInspector.h"
#include "raylib.h"
#include <vector>

class ColoringInteractionManager {
public:
    bool updateBackNavigation(Button& backButton) const;
    void updateColorButtons(std::vector<Button>& colorButtons, ColorPalette& colorPalette) const;
    void updateAnalysisControls(
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
        const std::vector<Button>& colorButtons) const;
    void updateDebugControls(
        ColoringInspector& inspector,
        Mandala& mandala,
        const Camera2D& camera,
        Camera2D& mutableCamera) const;

    int getRegionIdForColorSelection(
        Mandala& mandala,
        const Camera2D& camera,
        bool pointerOverUi,
        bool isDraggingCamera,
        bool analysisMode) const;

    bool isPointerOverUi(
        Vector2 screenPos,
        bool analysisMode,
        const Button& backButton,
        const Button& undoButton,
        const Button& validateButton,
        const Button& analysisButton,
        const Button& analysisCloseButton,
        const Button& analysisClearButton,
        const std::vector<Button>& colorButtons) const;
};
