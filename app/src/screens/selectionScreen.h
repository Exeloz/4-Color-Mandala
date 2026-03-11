#pragma once
#include "../game/gameState.h"
#include "../database/mandalaDatabase.h"
#include "../ui/button.h"
#include "../ui/confirmationDialog.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class SelectionScreen : public GameState {
public:
    SelectionScreen(std::shared_ptr<MandalaDatabase> database,
                    const std::unordered_set<int>& completedMandalaIdsNormal = {},
                    const std::unordered_set<int>& completedMandalaIdsHard = {},
                    const std::unordered_map<int, bool>& initialHardModeByMandalaId = {});

    void update(float deltaTime) override;
    void draw() override;
    std::shared_ptr<Mandala> getSelectedMandala() const;
    int consumeResetMandalaId();
    bool consumeResetMandalaHardMode();
    bool shouldTransitionToColoring() const;
    bool shouldTransitionToPalette() const;
    bool shouldReturnToStart() const;
    bool isSelectedMandalaHardMode() const;
    const std::unordered_map<int, bool>& getHardModeSelections() const;

private:
    std::shared_ptr<MandalaDatabase> database;
    std::vector<Button> mandalaButtons;
    std::vector<Button> resetButtons;
    std::vector<Button> modeButtons;
    std::shared_ptr<Mandala> selectedMandala;
    int selectedMandalaButtonIndex;
    bool selectedMandalaHardMode;
    int pendingResetMandalaId;
    int resetRequestedMandalaId;
    bool pendingResetMandalaHardMode;
    bool resetRequestedMandalaHardMode;
    bool transitionRequested;
    bool paletteRequested;
    bool returnRequested;
    Button paletteButton;
    Button backButton;
    ConfirmationDialog resetConfirmationDialog;
    std::unordered_set<int> completedMandalaIdsNormal;
    std::unordered_set<int> completedMandalaIdsHard;
    std::unordered_map<int, bool> hardModeByMandalaId;

    void layoutControls();
    void refreshModeButtonLabel(size_t index, int mandalaId);
};
