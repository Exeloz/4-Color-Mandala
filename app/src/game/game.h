#pragma once
#include "gameState.h"
#include "../database/mandalaDatabase.h"
#include "../screens/startScreen.h"
#include "../screens/dailyArchiveScreen.h"
#include "../screens/selectionScreen.h"
#include "../screens/paletteScreen.h"
#include "../screens/coloringScreen.h"
#include "../screens/winScreen.h"
#include "../ui/colorPalette.h"
#include "dailySelection.h"
#include "progressPersistence.h"
#include <memory>
#include <string>
#include <unordered_map>

class DailyRuleset;

enum class GameScreenState {
    START,
    ARCHIVE,
    SELECTION,
    PALETTE,
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
    std::shared_ptr<DailyArchiveScreen> dailyArchiveScreen;
    std::shared_ptr<SelectionScreen> selectionScreen;
    std::shared_ptr<PaletteScreen> paletteScreen;
    std::shared_ptr<ColoringScreen> coloringScreen;
    std::shared_ptr<WinScreen> winScreen;

    std::shared_ptr<Mandala> selectedMandala;
    std::vector<Color> appPaletteColors;
    std::unordered_map<int, bool> hardModeSelectionByMandalaId;
    bool hardModeEnabled;
    bool suppressWinTransition;
    bool transientRandomSession;
    GameScreenState transientSessionReturnState;
    std::string transientSessionKey;
    float transitionOverlayAlpha;
    ProgressPersistence progressPersistence;

    void transitionToState(GameScreenState newState);
    void updateCurrentState(float deltaTime);
    void drawCurrentState();
    std::shared_ptr<SelectionScreen> createSelectionScreen() const;
    void syncHardModeSelectionState();
    void savePaletteProgress();
    void saveSelectedMandalaProgress();
    void resetMandalaProgress(int mandalaId);
    std::vector<StatusBadge> createDailyRuleBadges(const DailyRuleset& ruleset) const;
    bool launchDailySession(uint64_t dateSeed,
                            const DailySelection& dailySelection,
                            GameScreenState returnState);
};
