#pragma once
#include "gameState.h"
#include "../database/mandalaDatabase.h"
#include "../screens/startScreen.h"
#include "../screens/selectionScreen.h"
#include "../screens/paletteScreen.h"
#include "../screens/coloringScreen.h"
#include "../screens/winScreen.h"
#include "../ui/colorPalette.h"
#include "progressPersistence.h"
#include <memory>
#include <unordered_map>

enum class GameScreenState {
    START,
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
    std::shared_ptr<SelectionScreen> selectionScreen;
    std::shared_ptr<PaletteScreen> paletteScreen;
    std::shared_ptr<ColoringScreen> coloringScreen;
    std::shared_ptr<WinScreen> winScreen;

    std::shared_ptr<Mandala> selectedMandala;
    std::vector<Color> appPaletteColors;
    std::unordered_map<int, bool> hardModeSelectionByMandalaId;
    bool hardModeEnabled;
    bool suppressWinTransition;
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
};
