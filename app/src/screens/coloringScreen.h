#pragma once
#include "../game/gameState.h"
#include "../mandala/mandala.h"
#include "../ui/colorPalette.h"
#include "../ui/button.h"
#include "coloringInspector.h"
#include "cameraInputManager.h"
#include "coloringActionManager.h"
#include "coloringInteractionManager.h"
#include "../rendering/fillPattern.h"
#include "raylib.h"
#include <memory>
#include <string>

struct RuleBadgeInfo {
    std::string label;        // short uppercase text shown on the badge
    std::string description;  // multi-line tooltip text (\n separator)
};

class ColoringScreen : public GameState {
public:
    ColoringScreen(std::shared_ptr<Mandala> mandala,
                  const std::vector<Color>& customPaletteColors = {},
                  bool readOnlyMode = false,
                  std::vector<RuleBadgeInfo> ruleBadges = {});

    void update(float deltaTime) override;
    void draw() override;
    bool isGameWon() const;
    bool shouldReturnToSelection() const;
    bool consumeSaveRequested();

private:
    std::shared_ptr<Mandala> mandala;
    ColorPalette colorPalette;
    std::vector<Button> colorButtons;
    Button backButton;
    Button undoButton;
    Button validateButton;
    Button analysisButton;
    Button analysisCloseButton;
    Button analysisClearButton;
    bool gameWon;
    bool returnRequested;
    ColoringInspector inspector;
    CameraInputManager cameraInputManager;
    ColoringActionManager actionManager;
    ColoringInteractionManager interactionManager;
    Camera2D camera;
    float zoom;
    FillPattern selectedPattern = FillPattern();
    int selectedAccentColorIndex = 0;
    float selectedPatternSize = 1.0f;
    int pendingColorChangesForSave;
    bool saveRequested;
    bool readOnlyMode;
    std::vector<RuleBadgeInfo> ruleBadges;
    // Popup state: index of the badge whose tooltip is open (-1 = none)
    int ruleBadgePopupIndex;
    std::vector<Rectangle> ruleBadgeRects;  // updated each draw(); used by update()

    void drawColorPalette();
    void drawRuleBadgePopup(const RuleBadgeInfo& badge, Rectangle badgeRect) const;
    void updatePatternControls();
    void applyCurrentPatternStyle(int regionId);
    void updateAnalysisInteractions();
    void updateDebugInteractions();
    void drawAnalysisOverlay() const;
    void fitCameraToMandala();
    void drawDebugOverlay() const;
    bool isPointerOverUi(Vector2 screenPos) const;
    void layoutTopButtons();
};
