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

SelectionScreen::SelectionScreen(std::shared_ptr<MandalaDatabase> database,
                                 const std::unordered_set<int>& completedMandalaIds)
        : database(database), selectedMandala(nullptr), selectedMandalaButtonIndex(-1), transitionRequested(false),
            paletteRequested(false), paletteButton(620, 20, 160, 50, "PALETTE"),
            completedMandalaIds(completedMandalaIds) {
    
    const auto& mandalaItems = database->getMandalaListItems();
    for (size_t i = 0; i < mandalaItems.size(); i++) {
        mandalaButtons.emplace_back(0.0f, 0.0f, 300.0f, 150.0f, mandalaItems[i].name);
        mandalaButtons.back().setTextScale(1.45f);
    }
}

void SelectionScreen::update(float deltaTime) {
    layoutControls();

    paletteButton.update();

    if (paletteButton.isClicked()) {
        paletteRequested = true;
        return;
    }

    const auto& mandalaItems = database->getMandalaListItems();
    for (size_t i = 0; i < mandalaButtons.size() && i < mandalaItems.size(); i++) {
        mandalaButtons[i].update();
        if (mandalaButtons[i].isClicked()) {
            const int selectedId = mandalaItems[i].id;
            database->loadMandala(selectedId);
            selectedMandala = database->getMandalaById(selectedId);
            if (selectedMandala != nullptr) {
                selectedMandalaButtonIndex = static_cast<int>(i);
                transitionRequested = true;
            }
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

    paletteButton.draw();

    const auto& mandalaItems = database->getMandalaListItems();
    for (size_t i = 0; i < mandalaButtons.size() && i < mandalaItems.size(); i++) {
        mandalaButtons[i].draw();

        if (static_cast<int>(i) == selectedMandalaButtonIndex) {
            DrawRectangleLinesEx(mandalaButtons[i].getBounds(), 5.0f, Colors::DarkBlue);
        }

        if (completedMandalaIds.count(mandalaItems[i].id) > 0) {
            Rectangle bounds = mandalaButtons[i].getBounds();
            float badgeWidth = 72.0f * uiScale;
            float badgeHeight = 24.0f * uiScale;
            float badgeX = bounds.x + bounds.width - badgeWidth - (10.0f * uiScale);
            float badgeY = bounds.y + (8.0f * uiScale);

            DrawRectangleRec({badgeX, badgeY, badgeWidth, badgeHeight}, Colors::Gold);
            DrawRectangleLinesEx({badgeX, badgeY, badgeWidth, badgeHeight}, 2.0f, Colors::Black);

            int doneTextSize = std::max(12, static_cast<int>(12.0f * uiScale));
            const char* doneLabel = "DONE";
            int doneTextWidth = MeasureText(doneLabel, doneTextSize);
            int doneTextX = static_cast<int>(badgeX + (badgeWidth - doneTextWidth) * 0.5f);
            int doneTextY = static_cast<int>(badgeY + (badgeHeight - doneTextSize) * 0.5f);
            DrawText(doneLabel, doneTextX, doneTextY, doneTextSize, Colors::Black);
        }
    }
}

void SelectionScreen::layoutControls() {
    float uiScale = getUiScale();
    bool mobileLayout = isMobileLayout();

    float sideMargin = 20.0f * uiScale;
    float topMargin = 20.0f * uiScale;

    float topButtonHeight = mobileLayout ? (58.0f * uiScale) : 50.0f;
    float paletteWidth = mobileLayout ? (190.0f * uiScale) : 160.0f;

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
    return false;
}
