#pragma once
#include "../game/gameState.h"
#include "../database/mandalaDatabase.h"
#include "../ui/button.h"
#include "../ui/confirmationDialog.h"
#include <memory>
#include <unordered_set>
#include <vector>

class SelectionScreen : public GameState {
public:
    SelectionScreen(std::shared_ptr<MandalaDatabase> database,
                    const std::unordered_set<int>& completedMandalaIds = {});

    void update(float deltaTime) override;
    void draw() override;
    std::shared_ptr<Mandala> getSelectedMandala() const;
    int consumeResetMandalaId();
    bool shouldTransitionToColoring() const;
    bool shouldTransitionToPalette() const;
    bool shouldReturnToStart() const;

private:
    std::shared_ptr<MandalaDatabase> database;
    std::vector<Button> mandalaButtons;
    std::vector<Button> resetButtons;
    std::shared_ptr<Mandala> selectedMandala;
    int selectedMandalaButtonIndex;
    int pendingResetMandalaId;
    int resetRequestedMandalaId;
    bool transitionRequested;
    bool paletteRequested;
    Button paletteButton;
    ConfirmationDialog resetConfirmationDialog;
    std::unordered_set<int> completedMandalaIds;

    void layoutControls();
};
