#include "winScreen.h"

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
    ClearBackground({240, 240, 240, 255});
    DrawText("YOU WIN!", 250, 80, 80, {0, 200, 0, 255});
    DrawText("Mandala completed with valid coloring!", 100, 180, 30, {0, 0, 0, 255});
    
    nextButton.draw();
    backToStartButton.draw();
}

bool WinScreen::shouldReturnToSelection() const {
    return returnToSelectionRequested;
}
