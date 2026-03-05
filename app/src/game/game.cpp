#include "game.h"

#include <algorithm>

namespace {
std::vector<Color> normalizePaletteSize(const std::vector<Color>& palette) {
    const std::vector<Color> defaults = ColorPalette().getColors();
    std::vector<Color> normalized = palette.empty() ? defaults : palette;

    if (normalized.size() < defaults.size()) {
        normalized.insert(normalized.end(), defaults.begin() + normalized.size(), defaults.end());
    }

    if (normalized.size() > defaults.size()) {
        normalized.resize(defaults.size());
    }

    return normalized;
}

std::vector<Color> buildPaletteForMode(const std::vector<Color>& fullPalette, bool hardMode) {
    if (hardMode) {
        return fullPalette;
    }

    const size_t normalSize = static_cast<size_t>(ColorPalette::LockedColorSlots + ColorPalette::NormalEditableColorCount);
    std::vector<Color> limited = fullPalette;
    if (limited.size() > normalSize) {
        limited.resize(normalSize);
    }
    return limited;
}
}

Game::Game() 
    : database(std::make_shared<MandalaDatabase>()), 
      currentState(GameScreenState::START),
            nextState(GameScreenState::START),
            appPaletteColors(ColorPalette().getColors()),
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

    progressPersistence.load();
    if (progressPersistence.hasPalette()) {
        appPaletteColors = normalizePaletteSize(progressPersistence.getPalette());
    } else {
        appPaletteColors = normalizePaletteSize(appPaletteColors);
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
                    syncHardModeSelectionState();
                    hardModeEnabled = selectionScreen->consumeResetMandalaHardMode();
                    resetMandalaProgress(resetMandalaId);
                    selectionScreen = createSelectionScreen();
                    break;
                }
            }

            if (selectionScreen->shouldTransitionToColoring()) {
                syncHardModeSelectionState();
                hardModeEnabled = selectionScreen->isSelectedMandalaHardMode();
                selectedMandala = selectionScreen->getSelectedMandala();
                bool openReadOnly = false;
                std::vector<Color> activeColorsForMandala = buildPaletteForMode(appPaletteColors, hardModeEnabled);

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
                                                                  openReadOnly);
                suppressWinTransition = false;
                transitionToState(GameScreenState::COLORING);
            }
            if (selectionScreen->shouldTransitionToPalette()) {
                syncHardModeSelectionState();
                paletteScreen = std::make_shared<PaletteScreen>(appPaletteColors);
                transitionToState(GameScreenState::PALETTE);
            }
            if (selectionScreen->shouldReturnToStart()) {
                transitionToState(GameScreenState::START);
            }
            break;

        case GameScreenState::PALETTE:
            if (paletteScreen->consumePaletteChanged()) {
                appPaletteColors = paletteScreen->getCustomizedColors();
                savePaletteProgress();
            }

            if (paletteScreen->shouldTransitionToColoring()) {
                appPaletteColors = paletteScreen->getCustomizedColors();
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
                                             progressPersistence.getCompletedMandalaIds(true),
                                             hardModeSelectionByMandalaId);
}

void Game::syncHardModeSelectionState() {
    if (selectionScreen == nullptr) {
        return;
    }

    hardModeSelectionByMandalaId = selectionScreen->getHardModeSelections();
}

void Game::savePaletteProgress() {
    progressPersistence.setPalette(appPaletteColors);
    progressPersistence.save();
}

void Game::saveSelectedMandalaProgress() {
    if (selectedMandala == nullptr) {
        return;
    }

    const std::string mandalaKey = ProgressPersistence::makeMandalaKey(selectedMandala->getId(), hardModeEnabled);

    std::vector<Color> paletteForCapture = appPaletteColors;
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
