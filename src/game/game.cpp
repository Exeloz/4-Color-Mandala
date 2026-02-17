#include "game.h"

Game::Game() 
    : database(std::make_shared<MandalaDatabase>()), 
      currentState(GameScreenState::START),
    nextState(GameScreenState::START),
    appPaletteColors(ColorPalette().getColors()) {}

Game::~Game() = default;

void Game::initialize() {
    database = std::make_shared<MandalaDatabase>();
    startScreen = std::make_shared<StartScreen>();
    selectionScreen = std::make_shared<SelectionScreen>(database);
    appPaletteColors = ColorPalette().getColors();
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
                coloringScreen = std::make_shared<ColoringScreen>(selectedMandala, appPaletteColors);
                transitionToState(GameScreenState::COLORING);
            }
            if (selectionScreen->shouldTransitionToPalette()) {
                paletteScreen = std::make_shared<PaletteScreen>(appPaletteColors);
                transitionToState(GameScreenState::PALETTE);
            }
            if (selectionScreen->shouldReturnToStart()) {
                transitionToState(GameScreenState::START);
            }
            break;

        case GameScreenState::PALETTE:
            if (paletteScreen->shouldTransitionToColoring()) {
                appPaletteColors = paletteScreen->getCustomizedColors();
                selectionScreen = std::make_shared<SelectionScreen>(database);
                transitionToState(GameScreenState::SELECTION);
            }
            if (paletteScreen->shouldReturnToSelection()) {
                selectionScreen = std::make_shared<SelectionScreen>(database);
                transitionToState(GameScreenState::SELECTION);
            }
            break;

        case GameScreenState::COLORING:
            if (coloringScreen->isGameWon()) {
                coloringScreen->saveWinImage();
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
        case GameScreenState::PALETTE:
            if (paletteScreen) paletteScreen->update(deltaTime);
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
        case GameScreenState::PALETTE:
            if (paletteScreen) paletteScreen->draw();
            break;
        case GameScreenState::COLORING:
            if (coloringScreen) coloringScreen->draw();
            break;
        case GameScreenState::WIN:
            if (winScreen) winScreen->draw();
            break;
    }
}
