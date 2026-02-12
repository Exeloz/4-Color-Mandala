#include "selectionScreen.h"
#include "../ui/colors.h"

SelectionScreen::SelectionScreen(std::shared_ptr<MandalaDatabase> database)
    : database(database), selectedMandala(nullptr), transitionRequested(false),
      returnToStartRequested(false), backButton(20, 20, 100, 50, "BACK") {
    
    const auto& mandalaList = database->getAllMandala();
    for (size_t i = 0; i < mandalaList.size(); i++) {
        float x = 100 + (i % 2) * 350;
        float y = 150 + (i / 2) * 200;
        mandalaButtons.emplace_back(x, y, 300, 150, mandalaList[i]->getName());
    }
}

void SelectionScreen::update(float deltaTime) {
    backButton.update();
    if (backButton.isClicked()) {
        returnToStartRequested = true;
        return;
    }

    const auto& mandalaList = database->getAllMandala();
    for (size_t i = 0; i < mandalaButtons.size() && i < mandalaList.size(); i++) {
        mandalaButtons[i].update();
        if (mandalaButtons[i].isClicked()) {
            selectedMandala = mandalaList[i];
            transitionRequested = true;
        }
    }
}

void SelectionScreen::draw() {
    ClearBackground(Colors::Gainsboro);
    DrawText("Select a Mandala", 200, 20, 40, Colors::Black);
    
    backButton.draw();

    for (auto& button : mandalaButtons) {
        button.draw();
    }
}

std::shared_ptr<Mandala> SelectionScreen::getSelectedMandala() const {
    return selectedMandala;
}

bool SelectionScreen::shouldTransitionToColoring() const {
    return transitionRequested;
}

bool SelectionScreen::shouldReturnToStart() const {
    return returnToStartRequested;
}
