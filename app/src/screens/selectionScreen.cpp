#include "selectionScreen.h"
#include "../ui/colors.h"

SelectionScreen::SelectionScreen(std::shared_ptr<MandalaDatabase> database)
        : database(database), selectedMandala(nullptr), selectedMandalaButtonIndex(-1), transitionRequested(false),
            paletteRequested(false), returnToStartRequested(false),
            backButton(20, 20, 100, 50, "BACK"), paletteButton(620, 20, 160, 50, "PALETTE") {
    
    const auto& mandalaList = database->getAllMandala();
    for (size_t i = 0; i < mandalaList.size(); i++) {
        float x = 100 + (i % 2) * 350;
        float y = 150 + (i / 2) * 200;
        mandalaButtons.emplace_back(x, y, 300, 150, mandalaList[i]->getName());
    }
}

void SelectionScreen::update(float deltaTime) {
    backButton.update();
    paletteButton.update();

    if (backButton.isClicked()) {
        returnToStartRequested = true;
        return;
    }

    if (paletteButton.isClicked()) {
        paletteRequested = true;
        return;
    }

    const auto& mandalaList = database->getAllMandala();
    for (size_t i = 0; i < mandalaButtons.size() && i < mandalaList.size(); i++) {
        mandalaButtons[i].update();
        if (mandalaButtons[i].isClicked()) {
            selectedMandala = mandalaList[i];
            selectedMandalaButtonIndex = static_cast<int>(i);
            transitionRequested = true;
            return;
        }
    }
}

void SelectionScreen::draw() {
    ClearBackground(Colors::Gainsboro);
    DrawText("Select a Mandala", 200, 20, 40, Colors::Black);
    
    backButton.draw();
    paletteButton.draw();

    for (size_t i = 0; i < mandalaButtons.size(); i++) {
        mandalaButtons[i].draw();

        if (static_cast<int>(i) == selectedMandalaButtonIndex) {
            DrawRectangleLinesEx(mandalaButtons[i].getBounds(), 5.0f, Colors::DarkBlue);
        }
    }
}

std::shared_ptr<Mandala> SelectionScreen::getSelectedMandala() const {
    return selectedMandala;
}

bool SelectionScreen::shouldTransitionToColoring() const {
    return transitionRequested;
}

bool SelectionScreen::shouldTransitionToPalette() const {
    return paletteRequested;
}

bool SelectionScreen::shouldReturnToStart() const {
    return returnToStartRequested;
}
