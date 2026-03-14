#include "game.h"

#include <algorithm>

void Game::update(float deltaTime) {
    transitionOverlayAlpha = std::max(0.0f, transitionOverlayAlpha - deltaTime * 2.8f);
    updateCurrentState(deltaTime);

    switch (currentState) {
        case GameScreenState::START:
            handleStartState();
            break;

        case GameScreenState::ARCHIVE:
            handleArchiveState();
            break;

        case GameScreenState::SELECTION:
            handleSelectionState();
            break;

        case GameScreenState::PALETTE:
            handlePaletteState();
            break;

        case GameScreenState::COLORING:
            handleColoringState();
            break;

        case GameScreenState::WIN:
            handleWinState();
            break;
    }
}

void Game::updateCurrentState(float deltaTime) {
    switch (currentState) {
        case GameScreenState::START:
            if (startScreen) startScreen->update(deltaTime);
            break;
        case GameScreenState::ARCHIVE:
            if (dailyArchiveScreen) dailyArchiveScreen->update(deltaTime);
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
        case GameScreenState::ARCHIVE:
            if (dailyArchiveScreen) dailyArchiveScreen->draw();
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

    if (transitionOverlayAlpha > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade({243, 246, 251, 255}, transitionOverlayAlpha));
    }
}
