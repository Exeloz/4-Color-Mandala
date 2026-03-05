#include "game.h"

Game::Game() 
    : database(std::make_shared<MandalaDatabase>()), 
      currentState(GameScreenState::START),
            nextState(GameScreenState::START),
            appPaletteColors(ColorPalette().getColors()),
            appPaletteColorsHard(ColorPalette().getColors()),
            hardModeEnabled(false),
            suppressWinTransition(false) {}

Game::~Game() {
    savePaletteProgress();
    progressPersistence.save();
}

void Game::initialize() {
    database = std::make_shared<MandalaDatabase>();
    startScreen = std::make_shared<StartScreen>();
    appPaletteColors = ColorPalette().getColors();
    appPaletteColorsHard = ColorPalette().getColors();

    progressPersistence.load();
    if (progressPersistence.hasPalette()) {
        appPaletteColors = progressPersistence.getPalette();
    }
    if (progressPersistence.hasPalette(true)) {
        appPaletteColorsHard = progressPersistence.getPalette(true);
    }

    selectionScreen = createSelectionScreen();
}

void Game::update(float deltaTime) {
    updateCurrentState(deltaTime);

    switch (currentState) {
        case GameScreenState::START:
            if (startScreen->shouldTransitionToSelection()) {
                selectionScreen = createSelectionScreen();
                transitionToState(GameScreenState::SELECTION);
            }
            break;

        case GameScreenState::SELECTION:
            {
                const int resetMandalaId = selectionScreen->consumeResetMandalaId();
                if (resetMandalaId >= 0) {
                    hardModeEnabled = selectionScreen->consumeResetMandalaHardMode();
                    resetMandalaProgress(resetMandalaId);
                    selectionScreen = createSelectionScreen();
                    break;
                }
            }

            if (selectionScreen->shouldTransitionToColoring()) {
                hardModeEnabled = selectionScreen->isSelectedMandalaHardMode();
                selectedMandala = selectionScreen->getSelectedMandala();
                bool openReadOnly = false;
                std::vector<Color> activeColorsForMandala = hardModeEnabled ? appPaletteColorsHard : appPaletteColors;

                if (selectedMandala != nullptr) {
                    const int selectedMandalaId = selectedMandala->getId();
                    const std::string mandalaKey = ProgressPersistence::makeMandalaKey(selectedMandalaId, hardModeEnabled);
                    openReadOnly = progressPersistence.isMandalaCompleted(mandalaKey);
                    if (openReadOnly) {
                        std::vector<Color> frozenPalette;
                        if (progressPersistence.tryGetMandalaFrozenPalette(mandalaKey, frozenPalette)
                            && !frozenPalette.empty()) {
                            activeColorsForMandala = frozenPalette;
                        }
                    }
                    progressPersistence.applyToMandala(mandalaKey, selectedMandala);
                }

                coloringScreen = std::make_shared<ColoringScreen>(selectedMandala,
                                                                  activeColorsForMandala,
                                                                  openReadOnly,
                                                                  hardModeEnabled);
                suppressWinTransition = false;
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
            if (paletteScreen->consumePaletteChanged()) {
                if (hardModeEnabled) {
                    appPaletteColorsHard = paletteScreen->getCustomizedColors();
                } else {
                    appPaletteColors = paletteScreen->getCustomizedColors();
                }
                savePaletteProgress();
            }

            if (paletteScreen->shouldTransitionToColoring()) {
                if (hardModeEnabled) {
                    appPaletteColorsHard = paletteScreen->getCustomizedColors();
                } else {
                    appPaletteColors = paletteScreen->getCustomizedColors();
                }
                savePaletteProgress();
                selectionScreen = createSelectionScreen();
                transitionToState(GameScreenState::SELECTION);
            }
            if (paletteScreen->shouldReturnToSelection()) {
                selectionScreen = createSelectionScreen();
                transitionToState(GameScreenState::SELECTION);
            }
            break;

        case GameScreenState::COLORING:
            if (coloringScreen->consumeSaveRequested()) {
                saveSelectedMandalaProgress();
            }

            if (coloringScreen->isGameWon() && !suppressWinTransition) {
                saveSelectedMandalaProgress();
                winScreen = std::make_shared<WinScreen>();
                transitionToState(GameScreenState::WIN);
            }
            if (coloringScreen->shouldReturnToSelection()) {
                selectionScreen = createSelectionScreen();
                transitionToState(GameScreenState::SELECTION);
            }
            break;

        case GameScreenState::WIN:
            if (winScreen->shouldReturnToColoring()) {
                if (selectedMandala != nullptr
                    && progressPersistence.isMandalaCompleted(
                        ProgressPersistence::makeMandalaKey(selectedMandala->getId(), hardModeEnabled))) {
                    suppressWinTransition = false;
                    selectionScreen = createSelectionScreen();
                    transitionToState(GameScreenState::SELECTION);
                    break;
                }
                suppressWinTransition = true;
                transitionToState(GameScreenState::COLORING);
            }
            if (winScreen->shouldReturnToSelection()) {
                suppressWinTransition = false;
                saveSelectedMandalaProgress();
                selectionScreen = createSelectionScreen();
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

std::shared_ptr<SelectionScreen> Game::createSelectionScreen() const {
    return std::make_shared<SelectionScreen>(database,
                                             progressPersistence.getCompletedMandalaIds(false),
                                             progressPersistence.getCompletedMandalaIds(true));
}

void Game::savePaletteProgress() {
    progressPersistence.setPalette(appPaletteColors, false);
    progressPersistence.setPalette(appPaletteColorsHard, true);
    progressPersistence.save();
}

void Game::saveSelectedMandalaProgress() {
    if (selectedMandala == nullptr) {
        return;
    }

    const std::string mandalaKey = ProgressPersistence::makeMandalaKey(selectedMandala->getId(), hardModeEnabled);

    std::vector<Color> paletteForCapture = hardModeEnabled ? appPaletteColorsHard : appPaletteColors;
    if (progressPersistence.isMandalaCompleted(mandalaKey)) {
        std::vector<Color> frozenPalette;
        if (progressPersistence.tryGetMandalaFrozenPalette(mandalaKey, frozenPalette)
            && !frozenPalette.empty()) {
            paletteForCapture = frozenPalette;
        }
    }

    progressPersistence.captureMandalaState(mandalaKey, *selectedMandala, paletteForCapture);
    progressPersistence.save();
}

void Game::resetMandalaProgress(int mandalaId) {
    const std::string mandalaKey = ProgressPersistence::makeMandalaKey(mandalaId, hardModeEnabled);
    progressPersistence.clearMandalaState(mandalaKey);
    progressPersistence.save();

    std::shared_ptr<Mandala> mandala = database->getMandalaById(mandalaId, hardModeEnabled);
    if (mandala != nullptr) {
        for (const Region& regionView : mandala->getRegions()) {
            if (!regionView.isColorable()) {
                continue;
            }

            Region* mutableRegion = mandala->getRegionById(regionView.getId());
            if (mutableRegion != nullptr) {
                mutableRegion->setColor(-1);
            }
        }
    }

    if (selectedMandala != nullptr && selectedMandala->getId() == mandalaId) {
        selectedMandala = nullptr;
    }
}
