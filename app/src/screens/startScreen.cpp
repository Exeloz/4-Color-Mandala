#include "startScreen.h"
#include "../ui/colors.h"
#include "raymath.h"
#include <algorithm>

namespace {
float getUiScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / 860.0f;
    float heightScale = static_cast<float>(GetScreenHeight()) / 420.0f;
    return Clamp(std::min(widthScale, heightScale), 0.75f, 2.4f);
}
}

StartScreen::StartScreen()
    : startButton(300, 250, 200, 100, "START"), transitionRequested(false) {}

void StartScreen::update(float deltaTime) {
    layoutControls();

    startButton.update();
    if (startButton.isClicked()) {
        transitionRequested = true;
    }
}

void StartScreen::draw() {
    ClearBackground(Colors::Gainsboro);

    float uiScale = getUiScale();
    int titleSize = static_cast<int>(58.0f * uiScale);
    const char* title = "Color Mandala";
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GetScreenWidth() - titleWidth) / 2;
    int titleY = static_cast<int>(80.0f * uiScale);
    DrawText(title, titleX, titleY, titleSize, Colors::Black);

    startButton.draw();
}

void StartScreen::layoutControls() {
    float uiScale = getUiScale();
    float buttonWidth = 220.0f * uiScale;
    float buttonHeight = 88.0f * uiScale;
    float buttonX = (GetScreenWidth() - buttonWidth) * 0.5f;
    float buttonY = (GetScreenHeight() - buttonHeight) * 0.62f;

    startButton.setPosition(buttonX, buttonY);
    startButton.setSize(buttonWidth, buttonHeight);
}

bool StartScreen::shouldTransitionToSelection() const {
    return transitionRequested;
}
