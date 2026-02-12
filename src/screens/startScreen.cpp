#include "startScreen.h"

StartScreen::StartScreen()
    : startButton(300, 250, 200, 100, "START"), transitionRequested(false) {}

void StartScreen::update(float deltaTime) {
    startButton.update();
    if (startButton.isClicked()) {
        transitionRequested = true;
    }
}

void StartScreen::draw() {
    ClearBackground({240, 240, 240, 255});
    DrawText("Color Mandala", 150, 50, 60, {0, 0, 0, 255});
    startButton.draw();
}

bool StartScreen::shouldTransitionToSelection() const {
    return transitionRequested;
}
