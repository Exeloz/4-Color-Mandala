#pragma once
#include "../game/gameState.h"
#include "../ui/button.h"

class WinScreen : public GameState {
public:
    WinScreen();

    void update(float deltaTime) override;
    void draw() override;
    bool shouldReturnToSelection() const;

private:
    Button nextButton;
    Button backToStartButton;
    bool returnToSelectionRequested;
    bool returnToStartRequested;
};
