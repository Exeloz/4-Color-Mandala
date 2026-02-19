#include "selectionScreen.h"
#include "../ui/colors.h"
#include "raymath.h"

namespace {
bool isMobileLayout() {
    return true;
}

float getUiScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / 860.0f;
    float heightScale = static_cast<float>(GetScreenHeight()) / 420.0f;
    return Clamp(std::min(widthScale, heightScale), 0.75f, 2.4f);
}
}

SelectionScreen::SelectionScreen(std::shared_ptr<MandalaDatabase> database)
        : database(database), selectedMandala(nullptr), selectedMandalaButtonIndex(-1), transitionRequested(false),
            paletteRequested(false), returnToStartRequested(false),
            backButton(20, 20, 100, 50, "BACK"), paletteButton(620, 20, 160, 50, "PALETTE") {
    
    const auto& mandalaList = database->getAllMandala();
    for (size_t i = 0; i < mandalaList.size(); i++) {
        mandalaButtons.emplace_back(0.0f, 0.0f, 300.0f, 150.0f, mandalaList[i]->getName());
    }
}

void SelectionScreen::update(float deltaTime) {
    layoutControls();

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

    float uiScale = getUiScale();
    int titleSize = std::max(static_cast<int>(34.0f), static_cast<int>(48.0f * uiScale));
    const char* title = "Select a Mandala";
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GetScreenWidth() - titleWidth) / 2;
    int titleY = static_cast<int>(20.0f * uiScale);
    DrawText(title, titleX, titleY, titleSize, Colors::Black);
    
    backButton.draw();
    paletteButton.draw();

    for (size_t i = 0; i < mandalaButtons.size(); i++) {
        mandalaButtons[i].draw();

        if (static_cast<int>(i) == selectedMandalaButtonIndex) {
            DrawRectangleLinesEx(mandalaButtons[i].getBounds(), 5.0f, Colors::DarkBlue);
        }
    }
}

void SelectionScreen::layoutControls() {
    float uiScale = getUiScale();
    bool mobileLayout = isMobileLayout();

    float sideMargin = 20.0f * uiScale;
    float topMargin = 20.0f * uiScale;

    float topButtonHeight = mobileLayout ? (58.0f * uiScale) : 50.0f;
    float backWidth = mobileLayout ? (136.0f * uiScale) : 100.0f;
    float paletteWidth = mobileLayout ? (190.0f * uiScale) : 160.0f;

    backButton.setPosition(sideMargin, topMargin);
    backButton.setSize(backWidth, topButtonHeight);

    paletteButton.setPosition(GetScreenWidth() - sideMargin - paletteWidth, topMargin);
    paletteButton.setSize(paletteWidth, topButtonHeight);

    if (mandalaButtons.empty()) {
        return;
    }

    float contentTop = topMargin + topButtonHeight + (24.0f * uiScale);
    float contentBottomMargin = 20.0f * uiScale;
    float availableHeight = static_cast<float>(GetScreenHeight()) - contentTop - contentBottomMargin;
    float availableWidth = static_cast<float>(GetScreenWidth()) - (2.0f * sideMargin);

    if (mobileLayout) {
        bool landscape = GetScreenWidth() >= GetScreenHeight();
        if (landscape) {
            float columnGap = 14.0f * uiScale;
            float rowGap = 14.0f * uiScale;
            float cardWidth = (availableWidth - columnGap) * 0.5f;
            int rows = static_cast<int>((mandalaButtons.size() + 1) / 2);
            float cardHeight = (availableHeight - rowGap * std::max(0, rows - 1)) / std::max(1, rows);
            cardHeight = std::max(cardHeight, 72.0f * uiScale);

            for (size_t i = 0; i < mandalaButtons.size(); i++) {
                int col = static_cast<int>(i % 2);
                int row = static_cast<int>(i / 2);
                float x = sideMargin + col * (cardWidth + columnGap);
                float y = contentTop + row * (cardHeight + rowGap);
                mandalaButtons[i].setPosition(x, y);
                mandalaButtons[i].setSize(cardWidth, cardHeight);
            }
            return;
        }

        int count = static_cast<int>(mandalaButtons.size());
        float gap = 16.0f * uiScale;
        float cardHeight = (availableHeight - (gap * (count - 1))) / std::max(1, count);
        cardHeight = std::max(cardHeight, 88.0f * uiScale);

        for (int i = 0; i < count; i++) {
            float y = contentTop + i * (cardHeight + gap);
            mandalaButtons[i].setPosition(sideMargin, y);
            mandalaButtons[i].setSize(availableWidth, cardHeight);
        }
        return;
    }

    float columnGap = 24.0f;
    float rowGap = 24.0f;
    float cardWidth = (availableWidth - columnGap) * 0.5f;
    float cardHeight = 150.0f;

    for (size_t i = 0; i < mandalaButtons.size(); i++) {
        int col = static_cast<int>(i % 2);
        int row = static_cast<int>(i / 2);
        float x = sideMargin + col * (cardWidth + columnGap);
        float y = contentTop + row * (cardHeight + rowGap);
        mandalaButtons[i].setPosition(x, y);
        mandalaButtons[i].setSize(cardWidth, cardHeight);
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
