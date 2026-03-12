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
    bool consumeTransitionToArchive();

private:
    Button startButton;
    Button dailyButton;
    Button archiveButton;
    bool transitionRequested;
    bool dailyRequested;
    bool archiveRequested;

    void layoutControls();
};
