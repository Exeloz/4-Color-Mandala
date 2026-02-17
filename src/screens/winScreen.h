#pragma once
#include "../game/gameState.h"
#include "../ui/button.h"

class WinScreen : public GameState {
public:
    WinScreen();

    void update(float deltaTime) override;
    void draw() override;
    bool shouldReturnToColoring() const;
    bool shouldReturnToSelection() const;

private:
    Button viewMandalaButton;
    Button menuButton;
    bool returnToColoringRequested;
    bool returnToSelectionRequested;
};
