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
    bool gameWon;
    bool returnRequested;
    Camera2D camera;
    float zoom;
    bool debugAdjacencyMode;
    int debugInspectRegionId;
    int debugHoverRegionId;
    std::set<std::pair<int, int>> debugSuggestedAdds;
    std::set<std::pair<int, int>> debugSuggestedRemoves;

    void drawColorPalette();
    void handleColorSelection();
    void updateZoom();
    void updateDebugOverlay();
    void drawDebugOverlay() const;
    int getRegionIdAtWorldPosition(Vector2 worldPos) const;
    void logAdjacencySuggestion(bool shouldExist, int regionA, int regionB);
};
