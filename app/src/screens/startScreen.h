#pragma once
#include "../game/gameState.h"
#include "../ui/button.h"
#include <memory>

class StartScreen : public GameState {
public:
    StartScreen();

    void update(float deltaTime) override;
    void draw() override;
    bool shouldTransitionToSelection() const;

private:
    Button startButton;
    bool transitionRequested;

    void layoutControls();
};
