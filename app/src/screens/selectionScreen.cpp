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
                                 const std::unordered_set<int>& completedMandalaIdsNormal,
                                 const std::unordered_set<int>& completedMandalaIdsHard,
                                 const std::unordered_map<int, bool>& initialHardModeByMandalaId)
        : database(database), selectedMandala(nullptr), selectedMandalaButtonIndex(-1), pendingResetMandalaId(-1),
            selectedMandalaHardMode(false), pendingResetMandalaHardMode(false), resetRequestedMandalaHardMode(false),
            resetRequestedMandalaId(-1), transitionRequested(false), paletteRequested(false), returnRequested(false),
            paletteButton(620, 20, 160, 50, "PALETTE"),
            backButton(20, 20, 160, 50, "BACK"),
            completedMandalaIdsNormal(completedMandalaIdsNormal),
            completedMandalaIdsHard(completedMandalaIdsHard) {
    
    const auto& mandalaItems = database->getMandalaListItems();
    for (size_t i = 0; i < mandalaItems.size(); i++) {
        mandalaButtons.emplace_back(0.0f, 0.0f, 300.0f, 150.0f, mandalaItems[i].name);
        mandalaButtons.back().setTextScale(1.45f);

        resetButtons.emplace_back(0.0f, 0.0f, 96.0f, 40.0f, "RESET");
        resetButtons.back().setTextScale(0.85f);

        modeButtons.emplace_back(0.0f, 0.0f, 96.0f, 40.0f, "MODE: N");
        modeButtons.back().setTextScale(0.75f);

        bool initialHardMode = false;
        auto it = initialHardModeByMandalaId.find(mandalaItems[i].id);
        if (it != initialHardModeByMandalaId.end()) {
            initialHardMode = it->second;
        }

        hardModeByMandalaId[mandalaItems[i].id] = mandalaItems[i].hasHardMode && initialHardMode;
        refreshModeButtonLabel(i, mandalaItems[i].id);
    }
}

void SelectionScreen::update(float deltaTime) {
    layoutControls();

    if (resetConfirmationDialog.isVisible()) {
        resetConfirmationDialog.update();

        if (resetConfirmationDialog.consumeConfirmed()) {
            if (pendingResetMandalaId >= 0) {
                resetRequestedMandalaId = pendingResetMandalaId;
                resetRequestedMandalaHardMode = pendingResetMandalaHardMode;
                if (pendingResetMandalaHardMode) {
                    completedMandalaIdsHard.erase(pendingResetMandalaId);
                } else {
                    completedMandalaIdsNormal.erase(pendingResetMandalaId);
                }
            }
            pendingResetMandalaId = -1;
            pendingResetMandalaHardMode = false;
            resetConfirmationDialog.hide();
        }

        if (resetConfirmationDialog.consumeCancelled()) {
            pendingResetMandalaId = -1;
            pendingResetMandalaHardMode = false;
            resetConfirmationDialog.hide();
        }
        return;
    }

    const auto& mandalaItems = database->getMandalaListItems();
    paletteButton.update();
    backButton.update();

    if (backButton.isClicked()) {
        returnRequested = true;
        return;
    }

    if (paletteButton.isClicked()) {
        paletteRequested = true;
        return;
    }

    for (size_t i = 0; i < mandalaButtons.size() && i < mandalaItems.size(); i++) {
        if (mandalaItems[i].hasHardMode) {
            modeButtons[i].update();
            if (modeButtons[i].isClicked()) {
                bool& hardEnabledForMandala = hardModeByMandalaId[mandalaItems[i].id];
                hardEnabledForMandala = !hardEnabledForMandala;
                refreshModeButtonLabel(i, mandalaItems[i].id);

                if (selectedMandalaButtonIndex == static_cast<int>(i)) {
                    selectedMandala = nullptr;
                    selectedMandalaButtonIndex = -1;
                    transitionRequested = false;
                }
                return;
            }
        }

        resetButtons[i].update();
        if (resetButtons[i].isClicked()) {
            pendingResetMandalaId = mandalaItems[i].id;
            pendingResetMandalaHardMode = mandalaItems[i].hasHardMode && hardModeByMandalaId[mandalaItems[i].id];
            std::string hardModeReset = pendingResetMandalaHardMode ? "(hard mode) " : "";
            resetConfirmationDialog.configure(
                "Reset Mandala",
                "Reset all progress for '" + mandalaItems[i].name + "' " + hardModeReset + "?",
                "Yes, Reset",
                "Cancel");
            resetConfirmationDialog.show();
            return;
        }

        mandalaButtons[i].update();
        if (mandalaButtons[i].isClicked()) {
            const int selectedId = mandalaItems[i].id;
            const bool loadHardMode = mandalaItems[i].hasHardMode && hardModeByMandalaId[selectedId];

            database->loadMandala(selectedId, loadHardMode);
            selectedMandala = database->getMandalaById(selectedId, loadHardMode);
            if (selectedMandala != nullptr) {
                selectedMandalaButtonIndex = static_cast<int>(i);
                selectedMandalaHardMode = loadHardMode;
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

    backButton.draw();
    paletteButton.draw();

    const auto& mandalaItems = database->getMandalaListItems();
    for (size_t i = 0; i < mandalaButtons.size() && i < mandalaItems.size(); i++) {
        mandalaButtons[i].draw();

        if (static_cast<int>(i) == selectedMandalaButtonIndex) {
            DrawRectangleLinesEx(mandalaButtons[i].getBounds(), 5.0f, Colors::DarkBlue);
        }

        const bool hardModeShown = mandalaItems[i].hasHardMode && hardModeByMandalaId[mandalaItems[i].id];
        const auto& completedSet = hardModeShown ? completedMandalaIdsHard : completedMandalaIdsNormal;
        if (completedSet.count(mandalaItems[i].id) > 0) {
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

        if (mandalaItems[i].hasHardMode) {
            modeButtons[i].draw();
        }
        resetButtons[i].draw();
    }

    resetConfirmationDialog.draw();
}

void SelectionScreen::layoutControls() {
    float uiScale = getUiScale();
    bool mobileLayout = isMobileLayout();

    float sideMargin = 20.0f * uiScale;
    float topMargin = 20.0f * uiScale;

    float topButtonHeight = mobileLayout ? (58.0f * uiScale) : 50.0f;
    float paletteWidth = mobileLayout ? (190.0f * uiScale) : 160.0f;

    backButton.setPosition(sideMargin, topMargin);
    backButton.setSize(paletteWidth, topButtonHeight);

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

                float resetWidth = std::max(92.0f, cardWidth * 0.23f);
                float resetHeight = std::max(34.0f, cardHeight * 0.30f);
                float modeWidth = std::max(108.0f, cardWidth * 0.30f);
                float actionGap = 8.0f * uiScale;
                resetButtons[i].setPosition(
                    x + cardWidth - resetWidth - (10.0f * uiScale),
                    y + cardHeight - resetHeight - (8.0f * uiScale));
                resetButtons[i].setSize(resetWidth, resetHeight);

                modeButtons[i].setPosition(
                    x + cardWidth - resetWidth - modeWidth - actionGap - (10.0f * uiScale),
                    y + cardHeight - resetHeight - (8.0f * uiScale));
                modeButtons[i].setSize(modeWidth, resetHeight);
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

            float resetWidth = std::max(96.0f, availableWidth * 0.22f);
            float resetHeight = std::max(34.0f, cardHeight * 0.28f);
            float modeWidth = std::max(112.0f, availableWidth * 0.26f);
            float actionGap = 8.0f * uiScale;
            resetButtons[i].setPosition(
                sideMargin + availableWidth - resetWidth - (10.0f * uiScale),
                y + cardHeight - resetHeight - (8.0f * uiScale));
            resetButtons[i].setSize(resetWidth, resetHeight);

            modeButtons[i].setPosition(
                sideMargin + availableWidth - resetWidth - modeWidth - actionGap - (10.0f * uiScale),
                y + cardHeight - resetHeight - (8.0f * uiScale));
            modeButtons[i].setSize(modeWidth, resetHeight);
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

        float resetWidth = std::max(92.0f, cardWidth * 0.23f);
        float resetHeight = std::max(34.0f, cardHeight * 0.28f);
        float modeWidth = std::max(112.0f, cardWidth * 0.28f);
        float actionGap = 8.0f;
        resetButtons[i].setPosition(
            x + cardWidth - resetWidth - 10.0f,
            y + cardHeight - resetHeight - 8.0f);
        resetButtons[i].setSize(resetWidth, resetHeight);

        modeButtons[i].setPosition(
            x + cardWidth - resetWidth - modeWidth - actionGap - 10.0f,
            y + cardHeight - resetHeight - 8.0f);
        modeButtons[i].setSize(modeWidth, resetHeight);
    }
}

std::shared_ptr<Mandala> SelectionScreen::getSelectedMandala() const {
    return selectedMandala;
}

int SelectionScreen::consumeResetMandalaId() {
    const int id = resetRequestedMandalaId;
    resetRequestedMandalaId = -1;
    return id;
}

bool SelectionScreen::consumeResetMandalaHardMode() {
    const bool hardMode = resetRequestedMandalaHardMode;
    resetRequestedMandalaHardMode = false;
    return hardMode;
}

bool SelectionScreen::shouldTransitionToColoring() const {
    return transitionRequested;
}

bool SelectionScreen::shouldTransitionToPalette() const {
    return paletteRequested;
}

bool SelectionScreen::shouldReturnToStart() const {
    return returnRequested;
}

bool SelectionScreen::isSelectedMandalaHardMode() const {
    return selectedMandalaHardMode;
}

const std::unordered_map<int, bool>& SelectionScreen::getHardModeSelections() const {
    return hardModeByMandalaId;
}

void SelectionScreen::refreshModeButtonLabel(size_t index, int mandalaId) {
    if (index >= modeButtons.size()) {
        return;
    }

    const bool hardEnabled = hardModeByMandalaId[mandalaId];
    modeButtons[index].setLabel(hardEnabled ? "MODE: H" : "MODE: N");
    if (hardEnabled) {
        modeButtons[index].setColors(Color{35, 125, 35, 255}, Color{50, 155, 50, 255}, Colors::LightGray, Colors::White);
    } else {
        modeButtons[index].setColors(Color{75, 75, 75, 255}, Color{105, 105, 105, 255}, Colors::LightGray, Colors::White);
    }
}
