#pragma once
#include "../game/gameState.h"
#include "../mandala/mandala.h"
#include "../ui/colorPalette.h"
#include "../ui/button.h"
#include "raylib.h"
#include <memory>
#include <set>
#include <utility>

class ColoringScreen : public GameState {
public:
    ColoringScreen(std::shared_ptr<Mandala> mandala);

    void update(float deltaTime) override;
    void draw() override;
    bool isGameWon() const;
    bool shouldReturnToSelection() const;

private:
    std::shared_ptr<Mandala> mandala;
    ColorPalette colorPalette;
    std::vector<Button> colorButtons;
    Button backButton;
    Button analysisButton;
    Button analysisCloseButton;
    Button analysisClearButton;
    bool gameWon;
    bool returnRequested;
    bool analysisMode;
    int analysisInspectRegionId;
    int analysisHoverRegionId;
    Camera2D camera;
    float zoom;
    bool isPanning;
    Vector2 lastPanPointer;
    bool debugAdjacencyMode;
    int debugInspectRegionId;
    int debugHoverRegionId;
    std::set<std::pair<int, int>> debugSuggestedAdds;
    std::set<std::pair<int, int>> debugSuggestedRemoves;

    void drawColorPalette();
    void handleColorSelection();
    void updatePan();
    void updateZoom();
    void updateAnalysisOverlay();
    void updateDebugOverlay();
    void drawAnalysisOverlay() const;
    void centerCameraOnRegion(int regionId);
    void drawDebugOverlay() const;
    int getRegionIdAtWorldPosition(Vector2 worldPos) const;
    bool isPointerOverUi(Vector2 screenPos) const;
    void layoutTopButtons();
    void logAdjacencySuggestion(bool shouldExist, int regionA, int regionB);
};
