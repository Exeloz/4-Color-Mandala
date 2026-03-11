#include "game.h"
#include "dailyRuleset.h"

#include <algorithm>
#include <cstdint>
#include <ctime>

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

struct DailySelection {
    int mandalaId;
    bool hardMode;
    int rulesetId = 0;  // id of the DailyRuleset chosen for this session
};

uint64_t mix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

uint64_t daySeedUtcIndependent() {
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    const uint64_t year = static_cast<uint64_t>(local->tm_year + 1900);
    const uint64_t month = static_cast<uint64_t>(local->tm_mon + 1);
    const uint64_t day = static_cast<uint64_t>(local->tm_mday);
    return year * 10000ULL + month * 100ULL + day;
}

uint64_t scoreCandidate(uint64_t daySeed, int mandalaId, bool hardMode, const DailyRuleset& ruleset) {
    uint64_t key = daySeed;
    key ^= static_cast<uint64_t>(mandalaId) * 0x9e3779b185ebca87ULL;
    key ^= hardMode ? 0xa0761d6478bd642fULL : 0xe7037ed1a0b428dbULL;
    key ^= ruleset.hashContribution();
    return mix64(key);
}

DailySelection chooseDailyMandalaForDay(const MandalaDatabase& database, uint64_t targetDate) {
    const auto& items = database.getMandalaListItems();
    const auto& rulesets = getAllDailyRulesets();

    DailySelection best{-1, false, 0};
    uint64_t bestScore = 0;
    bool hasBest = false;

    for (const auto& item : items) {
        if (item.id == 0) {
            // Skip tutorial mandala from Daily rotation.
            continue;
        }

        // Skip mandalas not yet published on the target date.
        if (item.availableFrom > 0 && static_cast<uint64_t>(item.availableFrom) > targetDate) {
            continue;
        }

        for (const auto& ruleset : rulesets) {
            const uint64_t normalScore = scoreCandidate(targetDate, item.id, false, *ruleset);
            if (!hasBest || normalScore > bestScore) {
                best = {item.id, false, ruleset->getId()};
                bestScore = normalScore;
                hasBest = true;
            }

            if (item.hasHardMode) {
                const uint64_t hardScore = scoreCandidate(targetDate, item.id, true, *ruleset);
                if (!hasBest || hardScore > bestScore) {
                    best = {item.id, true, ruleset->getId()};
                    bestScore = hardScore;
                    hasBest = true;
                }
            }
        }
    }

    return hasBest ? best : DailySelection{-1, false, 0};
}

std::string buildTransientSessionKey(uint64_t dateYYYYMMDD, int mandalaId, bool hardMode, int rulesetId) {
    const int year  = static_cast<int>(dateYYYYMMDD / 10000);
    const int month = static_cast<int>((dateYYYYMMDD / 100) % 100);
    const int day   = static_cast<int>(dateYYYYMMDD % 100);
    return "R_" + std::to_string(year)
           + "_" + std::to_string(month)
           + "_" + std::to_string(day)
           + "_" + std::to_string(mandalaId)
           + (hardMode ? "_H" : "_N")
           + "_" + std::to_string(rulesetId);
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
                transientSessionKey.clear();
                selectionScreen = createSelectionScreen();
                transitionToState(GameScreenState::SELECTION);
            } else if (startScreen->consumeTransitionToDaily()) {
                const DailySelection dailySelection = chooseDailyMandalaForDay(*database, daySeedUtcIndependent());
                if (dailySelection.mandalaId < 0) {
                    break;
                }

                hardModeEnabled = dailySelection.hardMode;
                database->loadMandala(dailySelection.mandalaId, hardModeEnabled);
                selectedMandala = cloneMandala(database->getMandalaById(dailySelection.mandalaId, hardModeEnabled));
                if (selectedMandala == nullptr) {
                    break;
                }

                // Daily session must start from a clean copy and never inherit normal save state.
                clearColorableRegions(selectedMandala);

                std::vector<Color> palette = buildPaletteForMode(appPaletteColors, hardModeEnabled);
                const uint64_t todaySeed = daySeedUtcIndependent();
                const DailyRuleset& ruleset = getDailyRulesetById(dailySelection.rulesetId);
                ruleset.applyToMandala(*selectedMandala, {todaySeed, dailySelection.mandalaId, hardModeEnabled});
                transientRandomSession = true;
                transientSessionKey = buildTransientSessionKey(
                    todaySeed, dailySelection.mandalaId, hardModeEnabled, dailySelection.rulesetId);
                progressPersistence.applyToMandala(transientSessionKey, selectedMandala);

                std::vector<StatusBadge> ruleBadges;
                {
                    const std::string shortLabel = ruleset.getShortLabel();
                    if (!shortLabel.empty()) {
                        ruleBadges.push_back({shortLabel, ruleset.getDescription(),
                            Color{147, 112, 219, 255},   // bgColor: MediumPurple
                            Color{ 80,  20, 160, 255},   // bgColorOpen: dark purple
                            Color{255, 255, 255, 255},   // textColor: white
                            Color{  0,   0,   0, 255}}); // borderColor: black
                    }
                }
                coloringScreen = std::make_shared<ColoringScreen>(selectedMandala, palette, false, std::move(ruleBadges));
                suppressWinTransition = false;
                transitionToState(GameScreenState::COLORING);
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
                if (wasTransientRandomSession) {
                    saveSelectedMandalaProgress();
                }
                transientRandomSession = false;
                transientSessionKey.clear();
                if (wasTransientRandomSession) {
                    if (startScreen == nullptr) {
                        startScreen = std::make_shared<StartScreen>();
                    }
                    transitionToState(GameScreenState::START);
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
                suppressWinTransition = false;
                saveSelectedMandalaProgress();
                transientRandomSession = false;
                transientSessionKey.clear();
                if (wasTransientRandomSession) {
                    if (startScreen == nullptr) {
                        startScreen = std::make_shared<StartScreen>();
                    }
                    transitionToState(GameScreenState::START);
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
