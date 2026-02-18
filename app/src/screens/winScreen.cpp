#include "winScreen.h"
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

WinScreen::WinScreen()
    : viewMandalaButton(180, 250, 240, 80, "VIEW MANDALA"),
      menuButton(440, 250, 180, 80, "MENU"),
      returnToColoringRequested(false), returnToSelectionRequested(false) {}

void WinScreen::update(float deltaTime) {
    layoutControls();

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

    float uiScale = getUiScale();
    int titleSize = static_cast<int>(78.0f * uiScale);
    const char* title = "YOU WIN!";
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GetScreenWidth() - titleWidth) / 2;
    int titleY = static_cast<int>(80.0f * uiScale);
    DrawText(title, titleX, titleY, titleSize, Colors::Green);

    int subtitleSize = static_cast<int>(28.0f * uiScale);
    const char* subtitle = "Mandala completed with valid coloring!";
    int subtitleWidth = MeasureText(subtitle, subtitleSize);
    int subtitleX = (GetScreenWidth() - subtitleWidth) / 2;
    int subtitleY = static_cast<int>(190.0f * uiScale);
    DrawText(subtitle, subtitleX, subtitleY, subtitleSize, Colors::Black);
    
    viewMandalaButton.draw();
    menuButton.draw();
}

void WinScreen::layoutControls() {
    float uiScale = getUiScale();
    float buttonWidth = 240.0f * uiScale;
    float buttonHeight = 72.0f * uiScale;
    float gap = 18.0f * uiScale;
    float startY = static_cast<float>(GetScreenHeight()) * 0.58f;

    if (GetScreenWidth() >= GetScreenHeight()) {
        float totalWidth = (buttonWidth * 2.0f) + gap;
        float startX = (GetScreenWidth() - totalWidth) * 0.5f;
        viewMandalaButton.setPosition(startX, startY);
        viewMandalaButton.setSize(buttonWidth, buttonHeight);
        menuButton.setPosition(startX + buttonWidth + gap, startY);
        menuButton.setSize(buttonWidth, buttonHeight);
        return;
    }

    float buttonX = (GetScreenWidth() - buttonWidth) * 0.5f;
    viewMandalaButton.setPosition(buttonX, startY);
    viewMandalaButton.setSize(buttonWidth, buttonHeight);
    menuButton.setPosition(buttonX, startY + buttonHeight + gap);
    menuButton.setSize(buttonWidth, buttonHeight);
}

bool WinScreen::shouldReturnToColoring() const {
    return returnToColoringRequested;
}

bool WinScreen::shouldReturnToSelection() const {
    return returnToSelectionRequested;
}
