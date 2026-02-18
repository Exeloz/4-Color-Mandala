#include "startScreen.h"
#include "../ui/colors.h"

StartScreen::StartScreen()
    : startButton(300, 250, 200, 100, "START"), transitionRequested(false) {}

void StartScreen::update(float deltaTime) {
    startButton.update();
    if (startButton.isClicked()) {
        transitionRequested = true;
    }
}

void StartScreen::draw() {
    ClearBackground(Colors::Gainsboro);
    DrawText("Color Mandala", 150, 50, 60, Colors::Black);
    startButton.draw();
}

bool StartScreen::shouldTransitionToSelection() const {
    return transitionRequested;
}
