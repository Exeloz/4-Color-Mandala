#pragma once
#include "../game/gameState.h"
#include "../mandala/mandala.h"
#include "../ui/colorPalette.h"
#include "../ui/button.h"
#include "coloringInspector.h"
#include "cameraInputManager.h"
#include "coloringActionManager.h"
#include "coloringInteractionManager.h"
#include "raylib.h"
#include <memory>

class ColoringScreen : public GameState {
public:
    ColoringScreen(std::shared_ptr<Mandala> mandala, const std::vector<Color>& customPaletteColors = {});

    void update(float deltaTime) override;
    void draw() override;
    bool isGameWon() const;
    bool shouldReturnToSelection() const;
    void saveWinImage();

private:
    std::shared_ptr<Mandala> mandala;
    ColorPalette colorPalette;
    std::vector<Button> colorButtons;
    Button backButton;
    Button undoButton;
    Button analysisButton;
    Button analysisCloseButton;
    Button analysisClearButton;
    bool winImageSaved;
    bool gameWon;
    bool returnRequested;
    ColoringInspector inspector;
    CameraInputManager cameraInputManager;
    ColoringActionManager actionManager;
    ColoringInteractionManager interactionManager;
    Camera2D camera;
    float zoom;

    void drawColorPalette();
    void updateAnalysisInteractions();
    void updateDebugInteractions();
    void drawAnalysisOverlay() const;
    void fitCameraToMandala();
    void drawDebugOverlay() const;
    bool isPointerOverUi(Vector2 screenPos) const;
    void layoutTopButtons();
};
