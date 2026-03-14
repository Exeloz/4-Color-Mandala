#include "game.h"

namespace {
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

void Game::handleStartState() {
    if (startScreen->consumeTransitionToSelection()) {
        transientRandomSession = false;
        transientSessionReturnState = GameScreenState::START;
        transientSessionKey.clear();
        selectionScreen = createSelectionScreen();
        transitionToState(GameScreenState::SELECTION);
    } else if (startScreen->consumeTransitionToDaily()) {
        const uint64_t todaySeed = getCurrentLocalDateSeed();
        const DailySelection dailySelection = chooseDailyMandalaForDay(*database, todaySeed);
        launchDailySession(todaySeed, dailySelection, GameScreenState::START);
    } else if (startScreen->consumeTransitionToArchive()) {
        dailyArchiveScreen = std::make_shared<DailyArchiveScreen>(database, progressPersistence, 20260301ULL);
        transitionToState(GameScreenState::ARCHIVE);
    }
}

void Game::handleArchiveState() {
    if (dailyArchiveScreen->consumeBackRequested()) {
        transitionToState(GameScreenState::START);
        return;
    }

    DailyArchiveLaunchRequest request;
    if (dailyArchiveScreen->consumeLaunchRequested(request)) {
        launchDailySession(request.dateSeed, request.selection, GameScreenState::ARCHIVE);
    }
}

void Game::handleSelectionState() {
    const int resetMandalaId = selectionScreen->consumeResetMandalaId();
    if (resetMandalaId >= 0) {
        syncHardModeSelectionState();
        hardModeEnabled = selectionScreen->consumeResetMandalaHardMode();
        resetMandalaProgress(resetMandalaId);
        selectionScreen = createSelectionScreen();
        return;
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
        transientRandomSession = false;
        transientSessionReturnState = GameScreenState::START;
        transientSessionKey.clear();
        suppressWinTransition = false;
        transitionToState(GameScreenState::COLORING);
    }
    if (selectionScreen->shouldTransitionToPalette()) {
        syncHardModeSelectionState();
        paletteScreen = std::make_shared<PaletteScreen>(appPaletteColors);
        transitionToState(GameScreenState::PALETTE);
    }
    if (selectionScreen->shouldReturnToStart()) {
        transientRandomSession = false;
        transientSessionReturnState = GameScreenState::START;
        transientSessionKey.clear();
        transitionToState(GameScreenState::START);
    }
}

void Game::handlePaletteState() {
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
}

void Game::handleColoringState() {
    if (coloringScreen->consumeSaveRequested()) {
        saveSelectedMandalaProgress();
    }

    if (coloringScreen->isGameWon() && !suppressWinTransition) {
        saveSelectedMandalaProgress();
        winScreen = std::make_shared<WinScreen>();
        transitionToState(GameScreenState::WIN);
    }
    if (coloringScreen->shouldReturnToSelection()) {
        const bool wasTransientRandomSession = transientRandomSession;
        const GameScreenState transientReturnState = transientSessionReturnState;
        if (wasTransientRandomSession) {
            saveSelectedMandalaProgress();
        }
        transientRandomSession = false;
        transientSessionReturnState = GameScreenState::START;
        transientSessionKey.clear();
        if (wasTransientRandomSession) {
            if (transientReturnState == GameScreenState::ARCHIVE) {
                dailyArchiveScreen = std::make_shared<DailyArchiveScreen>(database, progressPersistence, 20260301ULL);
                transitionToState(GameScreenState::ARCHIVE);
            } else {
                if (startScreen == nullptr) {
                    startScreen = std::make_shared<StartScreen>();
                }
                transitionToState(GameScreenState::START);
            }
        } else {
            selectionScreen = createSelectionScreen();
            transitionToState(GameScreenState::SELECTION);
        }
    }
}

void Game::handleWinState() {
    if (winScreen->shouldReturnToColoring()) {
        std::vector<Color> activeColorsForMandala = buildPaletteForMode(appPaletteColors, hardModeEnabled);
        if (selectedMandala != nullptr) {
            const std::string mandalaKey = transientRandomSession
                ? transientSessionKey
                : ProgressPersistence::makeMandalaKey(selectedMandala->getId(), hardModeEnabled);

            std::vector<Color> frozenPalette;
            if (progressPersistence.tryGetMandalaFrozenPalette(mandalaKey, frozenPalette)
                && !frozenPalette.empty()) {
                activeColorsForMandala = frozenPalette;
            }

            coloringScreen = std::make_shared<ColoringScreen>(selectedMandala,
                                                              activeColorsForMandala,
                                                              true);
        }
        suppressWinTransition = true;
        transitionToState(GameScreenState::COLORING);
    }
    if (winScreen->shouldReturnToSelection()) {
        const bool wasTransientRandomSession = transientRandomSession;
        const GameScreenState transientReturnState = transientSessionReturnState;
        suppressWinTransition = false;
        saveSelectedMandalaProgress();
        transientRandomSession = false;
        transientSessionReturnState = GameScreenState::START;
        transientSessionKey.clear();
        if (wasTransientRandomSession) {
            if (transientReturnState == GameScreenState::ARCHIVE) {
                dailyArchiveScreen = std::make_shared<DailyArchiveScreen>(database, progressPersistence, 20260301ULL);
                transitionToState(GameScreenState::ARCHIVE);
            } else {
                if (startScreen == nullptr) {
                    startScreen = std::make_shared<StartScreen>();
                }
                transitionToState(GameScreenState::START);
            }
        } else {
            selectionScreen = createSelectionScreen();
            transitionToState(GameScreenState::SELECTION);
        }
    }
}
