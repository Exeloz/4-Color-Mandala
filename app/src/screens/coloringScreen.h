#pragma once
#include "../game/gameState.h"
#include "../mandala/mandala.h"
#include "../ui/colorPalette.h"
#include "../ui/button.h"
#include "coloringInspector.h"
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
    Button analysisButton;
    Button analysisCloseButton;
    Button analysisClearButton;
    bool winImageSaved;
    bool gameWon;
    bool returnRequested;
    ColoringInspector inspector;
    Camera2D camera;
    float zoom;
    bool isPanning;
    bool isDraggingCamera;
    bool isPinching;
    float lastPinchDistance;
    Vector2 lastPanPointer;

    void drawColorPalette();
    void handleColorSelection();
    void updatePan();
    void updateZoom();
    void updateAnalysisOverlay();
    void updateDebugOverlay();
    void drawAnalysisOverlay() const;
    void fitCameraToMandala();
    void drawDebugOverlay() const;
    bool isPointerOverUi(Vector2 screenPos) const;
    void layoutTopButtons();
};
