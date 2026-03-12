#include "game.h"
#include "dailyRuleset.h"

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

std::shared_ptr<Mandala> cloneMandala(const std::shared_ptr<Mandala>& src) {
    if (!src) {
        return nullptr;
    }
    return std::make_shared<Mandala>(*src);
}

void clearColorableRegions(const std::shared_ptr<Mandala>& mandala) {
    if (mandala == nullptr) {
        return;
    }

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
}

Game::Game() 
    : database(std::make_shared<MandalaDatabase>()), 
      currentState(GameScreenState::START),
    nextState(GameScreenState::START),
    appPaletteColors(ColorPalette().getColors()),
    hardModeEnabled(false),
    suppressWinTransition(false),
    transientRandomSession(false),
        transientSessionReturnState(GameScreenState::START),
    transientSessionKey(),
    transitionOverlayAlpha(0.0f) {}

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
    transitionOverlayAlpha = std::max(0.0f, transitionOverlayAlpha - deltaTime * 2.8f);
    updateCurrentState(deltaTime);

    switch (currentState) {
        case GameScreenState::START:
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
            break;

        case GameScreenState::ARCHIVE:
            if (dailyArchiveScreen->consumeBackRequested()) {
                transitionToState(GameScreenState::START);
                break;
            }

            {
                DailyArchiveLaunchRequest request;
                if (dailyArchiveScreen->consumeLaunchRequested(request)) {
                    launchDailySession(request.dateSeed, request.selection, GameScreenState::ARCHIVE);
                }
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
            break;

        case GameScreenState::WIN:
            if (winScreen->shouldReturnToColoring()) {
                // View the completed colored mandala in read-only mode
                if (selectedMandala != nullptr) {
                    std::vector<Color> activeColorsForMandala = buildPaletteForMode(appPaletteColors, hardModeEnabled);
                    const std::string mandalaKey = transientRandomSession
                        ? transientSessionKey
                        : ProgressPersistence::makeMandalaKey(selectedMandala->getId(), hardModeEnabled);
        
                    // Use frozen palette if available for completed mandalas
                    std::vector<Color> frozenPalette;
                    if (progressPersistence.tryGetMandalaFrozenPalette(mandalaKey, frozenPalette)
                        && !frozenPalette.empty()) {
                        activeColorsForMandala = frozenPalette;
                    }
        
                    // Recreate the coloring screen in read-only mode
                    coloringScreen = std::make_shared<ColoringScreen>(selectedMandala,
                                                                        activeColorsForMandala,
                                                                        true);  // readOnlyMode = true
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
    transitionOverlayAlpha = 0.12f;
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

    const std::string mandalaKey = transientRandomSession
        ? transientSessionKey
        : ProgressPersistence::makeMandalaKey(selectedMandala->getId(), hardModeEnabled);

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

std::vector<StatusBadge> Game::createDailyRuleBadges(const DailyRuleset& ruleset) const {
    std::vector<StatusBadge> ruleBadges;
    const std::string shortLabel = ruleset.getShortLabel();
    if (!shortLabel.empty()) {
        ruleBadges.push_back({shortLabel, ruleset.getDescription(),
            Color{147, 112, 219, 255},
            Color{80, 20, 160, 255},
            Color{255, 255, 255, 255},
            Color{0, 0, 0, 255}});
    }
    return ruleBadges;
}

bool Game::launchDailySession(uint64_t dateSeed,
                              const DailySelection& dailySelection,
                              GameScreenState returnState) {
    if (dailySelection.mandalaId < 0) {
        return false;
    }

    hardModeEnabled = dailySelection.hardMode;
    database->loadMandala(dailySelection.mandalaId, hardModeEnabled);
    selectedMandala = cloneMandala(database->getMandalaById(dailySelection.mandalaId, hardModeEnabled));
    if (selectedMandala == nullptr) {
        return false;
    }

    clearColorableRegions(selectedMandala);

    std::vector<Color> palette = buildPaletteForMode(appPaletteColors, hardModeEnabled);
    const DailyRuleset& ruleset = getDailyRulesetById(dailySelection.rulesetId);
    ruleset.applyToMandala(*selectedMandala, {dateSeed, dailySelection.mandalaId, hardModeEnabled});

    transientRandomSession = true;
    transientSessionReturnState = returnState;
    transientSessionKey = buildTransientSessionKey(
        dateSeed, dailySelection.mandalaId, hardModeEnabled, dailySelection.rulesetId);

    progressPersistence.applyToMandala(transientSessionKey, selectedMandala);

    bool openReadOnly = progressPersistence.isMandalaCompleted(transientSessionKey);
    if (openReadOnly) {
        std::vector<Color> frozenPalette;
        if (progressPersistence.tryGetMandalaFrozenPalette(transientSessionKey, frozenPalette)
            && !frozenPalette.empty()) {
            palette = frozenPalette;
        }
    }

    coloringScreen = std::make_shared<ColoringScreen>(selectedMandala,
                                                      palette,
                                                      openReadOnly,
                                                      createDailyRuleBadges(ruleset));
    suppressWinTransition = false;
    transitionToState(GameScreenState::COLORING);
    return true;
}
