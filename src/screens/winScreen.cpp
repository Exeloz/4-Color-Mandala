#include "winScreen.h"
#include "../ui/colors.h"

WinScreen::WinScreen()
    : viewMandalaButton(180, 250, 240, 80, "VIEW MANDALA"),
      menuButton(440, 250, 180, 80, "MENU"),
      returnToColoringRequested(false), returnToSelectionRequested(false) {}

void WinScreen::update(float deltaTime) {
    viewMandalaButton.update();
    menuButton.update();

    if (viewMandalaButton.isClicked()) {
        returnToColoringRequested = true;
    }
    if (menuButton.isClicked()) {
        returnToSelectionRequested = true;
    }
}

void WinScreen::draw() {
    ClearBackground(Colors::Gainsboro);
    DrawText("YOU WIN!", 250, 80, 80, Colors::Green);
    DrawText("Mandala completed with valid coloring!", 100, 180, 30, Colors::Black);
    
    viewMandalaButton.draw();
    menuButton.draw();
}

bool WinScreen::shouldReturnToColoring() const {
    return returnToColoringRequested;
}

bool WinScreen::shouldReturnToSelection() const {
    return returnToSelectionRequested;
}
