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
        : startButton(300, 250, 200, 100, "START"),
            dailyButton(300, 250, 200, 100, "DAILY"),
            archiveButton(300, 250, 200, 100, "ARCHIVE"),
            transitionRequested(false),
            dailyRequested(false),
            archiveRequested(false) {}

void StartScreen::update(float deltaTime) {
    layoutControls();

    startButton.update();
    if (startButton.isClicked()) {
        transitionRequested = true;
    }

    dailyButton.update();
    if (dailyButton.isClicked()) {
        dailyRequested = true;
    }

    archiveButton.update();
    if (archiveButton.isClicked()) {
        archiveRequested = true;
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
    dailyButton.draw();
    archiveButton.draw();
}

void StartScreen::layoutControls() {
    float uiScale = getUiScale();
    float margin = 18.0f * uiScale;
    float buttonWidth = 220.0f * uiScale;
    float buttonHeight = 88.0f * uiScale;
    float buttonX = (GetScreenWidth() - buttonWidth) * 0.5f;
    float buttonY = (GetScreenHeight() - buttonHeight) * 0.56f;
    float gapY = 16.0f * uiScale;

    startButton.setPosition(buttonX, buttonY);
    startButton.setSize(buttonWidth, buttonHeight);

    dailyButton.setPosition(buttonX, buttonY + buttonHeight + gapY);
    dailyButton.setSize(buttonWidth, buttonHeight);
    dailyButton.setColors(Color{57, 96, 168, 255}, Color{86, 124, 194, 255}, Colors::Black, Colors::White);

    float archiveButtonWidth = buttonWidth * 0.5f;
    float archiveButtonHeight = buttonHeight * 0.5f;
    archiveButton.setPosition(GetScreenWidth() - margin - archiveButtonWidth,
                              GetScreenHeight() - margin - archiveButtonHeight);
    archiveButton.setSize(archiveButtonWidth, archiveButtonHeight);
    archiveButton.setTextScale(0.8f);
    archiveButton.setColors(Color{74, 87, 112, 255}, Color{97, 112, 140, 255}, Colors::Black, Colors::White);
}

bool StartScreen::consumeTransitionToSelection() {
    if (!transitionRequested) {
        return false;
    }

    transitionRequested = false;
    return true;
}

bool StartScreen::consumeTransitionToDaily() {
    if (!dailyRequested) {
        return false;
    }

    dailyRequested = false;
    return true;
}

bool StartScreen::consumeTransitionToArchive() {
    if (!archiveRequested) {
        return false;
    }

    archiveRequested = false;
    return true;
}
