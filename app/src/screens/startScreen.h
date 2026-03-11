#pragma once
#include "../game/gameState.h"
#include "../ui/button.h"
#include <memory>

class StartScreen : public GameState {
public:
    StartScreen();

    void update(float deltaTime) override;
    void draw() override;
    bool consumeTransitionToSelection();
    bool consumeTransitionToDaily();

private:
    Button startButton;
    Button dailyButton;
    bool transitionRequested;
    bool dailyRequested;

    void layoutControls();
};
