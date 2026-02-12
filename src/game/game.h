#pragma once
#include "gameState.h"
#include "../database/mandalaDatabase.h"
#include "../screens/startScreen.h"
#include "../screens/selectionScreen.h"
#include "../screens/coloringScreen.h"
#include "../screens/winScreen.h"
#include <memory>

enum class GameScreenState {
    START,
    SELECTION,
    COLORING,
    WIN
};

class Game {
public:
    Game();
    ~Game();

    void initialize();
    void update(float deltaTime);
    void draw();
    bool shouldClose() const;

private:
    std::shared_ptr<MandalaDatabase> database;
    GameScreenState currentState;
    GameScreenState nextState;
    
    std::shared_ptr<StartScreen> startScreen;
    std::shared_ptr<SelectionScreen> selectionScreen;
    std::shared_ptr<ColoringScreen> coloringScreen;
    std::shared_ptr<WinScreen> winScreen;

    std::shared_ptr<Mandala> selectedMandala;

    void transitionToState(GameScreenState newState);
    void updateCurrentState(float deltaTime);
    void drawCurrentState();
};
