#include "game.h"

Game::Game() 
    : database(std::make_shared<MandalaDatabase>()), 
      currentState(GameScreenState::START),
      nextState(GameScreenState::START) {}

Game::~Game() = default;

void Game::initialize() {
    database = std::make_shared<MandalaDatabase>();
    startScreen = std::make_shared<StartScreen>();
    selectionScreen = std::make_shared<SelectionScreen>(database);
}

void Game::update(float deltaTime) {
    updateCurrentState(deltaTime);

    switch (currentState) {
        case GameScreenState::START:
            if (startScreen->shouldTransitionToSelection()) {
                selectionScreen = std::make_shared<SelectionScreen>(database);
                transitionToState(GameScreenState::SELECTION);
            }
            break;

        case GameScreenState::SELECTION:
            if (selectionScreen->shouldTransitionToColoring()) {
                selectedMandala = selectionScreen->getSelectedMandala();
                coloringScreen = std::make_shared<ColoringScreen>(selectedMandala);
                transitionToState(GameScreenState::COLORING);
            }
            if (selectionScreen->shouldReturnToStart()) {
                transitionToState(GameScreenState::START);
            }
            break;

        case GameScreenState::COLORING:
            if (coloringScreen->isGameWon()) {
                winScreen = std::make_shared<WinScreen>();
                transitionToState(GameScreenState::WIN);
            }
            if (coloringScreen->shouldReturnToSelection()) {
                selectionScreen = std::make_shared<SelectionScreen>(database);
                transitionToState(GameScreenState::SELECTION);
            }
            break;

        case GameScreenState::WIN:
            if (winScreen->shouldReturnToSelection()) {
                selectionScreen = std::make_shared<SelectionScreen>(database);
                transitionToState(GameScreenState::SELECTION);
            }
            break;
    }
}

void Game::draw() {
    drawCurrentState();
}

bool Game::shouldClose() const {
    return WindowShouldClose();
}

void Game::transitionToState(GameScreenState newState) {
    currentState = newState;
}

void Game::updateCurrentState(float deltaTime) {
    switch (currentState) {
        case GameScreenState::START:
            if (startScreen) startScreen->update(deltaTime);
            break;
        case GameScreenState::SELECTION:
            if (selectionScreen) selectionScreen->update(deltaTime);
            break;
        case GameScreenState::COLORING:
            if (coloringScreen) coloringScreen->update(deltaTime);
            break;
        case GameScreenState::WIN:
            if (winScreen) winScreen->update(deltaTime);
            break;
    }
}

void Game::drawCurrentState() {
    switch (currentState) {
        case GameScreenState::START:
            if (startScreen) startScreen->draw();
            break;
        case GameScreenState::SELECTION:
            if (selectionScreen) selectionScreen->draw();
            break;
        case GameScreenState::COLORING:
            if (coloringScreen) coloringScreen->draw();
            break;
        case GameScreenState::WIN:
            if (winScreen) winScreen->draw();
            break;
    }
}
