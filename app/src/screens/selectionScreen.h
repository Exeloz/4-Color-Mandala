#pragma once
#include "../game/gameState.h"
#include "../database/mandalaDatabase.h"
#include "../ui/button.h"
#include <memory>
#include <vector>

class SelectionScreen : public GameState {
public:
    SelectionScreen(std::shared_ptr<MandalaDatabase> database);

    void update(float deltaTime) override;
    void draw() override;
    std::shared_ptr<Mandala> getSelectedMandala() const;
    bool shouldTransitionToColoring() const;
    bool shouldTransitionToPalette() const;
    bool shouldReturnToStart() const;

private:
    std::shared_ptr<MandalaDatabase> database;
    std::vector<Button> mandalaButtons;
    std::shared_ptr<Mandala> selectedMandala;
    int selectedMandalaButtonIndex;
    bool transitionRequested;
    bool paletteRequested;
    bool returnToStartRequested;
    Button backButton;
    Button paletteButton;

    void layoutControls();
};
