#include "winScreen.h"
#include "../ui/colors.h"

WinScreen::WinScreen()
    : nextButton(250, 250, 150, 80, "NEXT"),
      backToStartButton(450, 250, 150, 80, "MENU"),
      returnToSelectionRequested(false), returnToStartRequested(false) {}

void WinScreen::update(float deltaTime) {
    nextButton.update();
    backToStartButton.update();

    if (nextButton.isClicked()) {
        returnToSelectionRequested = true;
    }
    if (backToStartButton.isClicked()) {
        returnToStartRequested = true;
    }
}

void WinScreen::draw() {
    ClearBackground(Colors::Gainsboro);
    DrawText("YOU WIN!", 250, 80, 80, Colors::Green);
    DrawText("Mandala completed with valid coloring!", 100, 180, 30, Colors::Black);
    
    nextButton.draw();
    backToStartButton.draw();
}

bool WinScreen::shouldReturnToSelection() const {
    return returnToSelectionRequested;
}
