#pragma once
#include "../game/gameState.h"
#include "../ui/button.h"
#include "raylib.h"
#include <vector>

class PaletteScreen : public GameState {
public:
    PaletteScreen(const std::vector<Color>& initialPaletteColors = {});

    void update(float deltaTime) override;
    void draw() override;

    bool shouldTransitionToColoring() const;
    bool shouldReturnToSelection() const;
    const std::vector<Color>& getCustomizedColors() const;

private:
    std::vector<Color> paletteColors;
    std::vector<Color> availableColors;
    int activeSlotIndex;

    Button backButton;
    Button continueButton;
    Button prevPageButton;
    Button nextPageButton;

    bool transitionRequested;
    bool returnRequested;
    int swatchPage;

    void layoutControls();
    Rectangle getPaletteSlotBounds(int slotIndex) const;
    Rectangle getAvailableColorBounds(int colorIndex) const;
    int getSwatchStartIndex() const;
    int getSwatchEndIndex() const;
};
