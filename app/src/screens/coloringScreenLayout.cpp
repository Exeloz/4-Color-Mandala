#include "coloringScreen.h"

#include "raymath.h"

#include <algorithm>

namespace {
bool isMobileLayout() {
    return true;
}

float getUiScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / 860.0f;
    float heightScale = static_cast<float>(GetScreenHeight()) / 420.0f;
    return Clamp(std::min(widthScale, heightScale), 0.75f, 2.4f);
}

int getAvailableEditableColorCount(const Mandala& mandala, int totalCount) {
    const int editableCount = std::max(0, totalCount - ColorPalette::LockedColorSlots);

    const int minimumRequired = mandala.getMinimumColorCount();
    if (minimumRequired > 0) {
        return std::min(editableCount, minimumRequired);
    }

    if (mandala.isHardMode()) {
        return editableCount;
    }

    return std::min(editableCount, ColorPalette::NormalEditableColorCount);
}
}

void ColoringScreen::layoutTopButtons() {
    float uiScale = getUiScale();
    bool mobileLayout = isMobileLayout();

    float leftMargin = 20.0f * uiScale;
    float rightMargin = 20.0f * uiScale;
    float topMargin = 20.0f * uiScale;

    float topButtonHeight = mobileLayout ? 56.0f * uiScale : 50.0f;
    float backButtonWidth = mobileLayout ? 136.0f * uiScale : 100.0f;
    float mainButtonWidth = mobileLayout ? 150.0f * uiScale : 150.0f;
    float clearButtonWidth = mobileLayout ? 86.0f * uiScale : 70.0f;
    float clearButtonHeight = mobileLayout ? 44.0f * uiScale : 44.0f;

    backButton.setPosition(leftMargin, topMargin);
    backButton.setSize(backButtonWidth, topButtonHeight);

    float undoButtonWidth = mobileLayout ? 126.0f * uiScale : 110.0f;
    float undoButtonHeight = topButtonHeight;
    if (readOnlyMode) {
        undoButton.setPosition(0.0f, 0.0f);
        undoButton.setSize(0.0f, 0.0f);
    } else {
        float undoButtonX = GetScreenWidth() - undoButtonWidth - rightMargin;
        float undoButtonY = GetScreenHeight() - undoButtonHeight - (20.0f * uiScale);
        undoButton.setPosition(undoButtonX, undoButtonY);
        undoButton.setSize(undoButtonWidth, undoButtonHeight);
    }

    float validateButtonX = GetScreenWidth() - 2 * mainButtonWidth - rightMargin;
    if (readOnlyMode) {
        validateButton.setPosition(0.0f, 0.0f);
        validateButton.setSize(0.0f, 0.0f);
    } else {
        validateButton.setPosition(validateButtonX, topMargin);
        validateButton.setSize(mainButtonWidth, topButtonHeight);
    }

    float controlsY = topMargin + topButtonHeight + (10.0f * uiScale);

    float mainButtonX = GetScreenWidth() - mainButtonWidth - rightMargin;
    analysisButton.setPosition(mainButtonX, topMargin);
    analysisButton.setSize(mainButtonWidth, topButtonHeight);
    analysisCloseButton.setPosition(mainButtonX, topMargin);
    analysisCloseButton.setSize(mainButtonWidth, topButtonHeight);

    analysisClearButton.setPosition(GetScreenWidth() - clearButtonWidth - rightMargin, controlsY);
    analysisClearButton.setSize(clearButtonWidth, clearButtonHeight);

    if (colorButtons.empty()) {
        return;
    }

    int colorCount = static_cast<int>(colorButtons.size());
    int availableEditableColorCount = getAvailableEditableColorCount(*mandala, colorCount);

    if (readOnlyMode) {
        for (int i = 0; i < colorCount; i++) {
            colorButtons[i].setPosition(0.0f, 0.0f);
            colorButtons[i].setSize(0.0f, 0.0f);
        }
        return;
    }

    const bool useTwoColumns = availableEditableColorCount >= 8;

    float paletteX = leftMargin;
    float paletteTop = controlsY;
    float paletteBottomMargin = 20.0f * uiScale;
    float availableHeight = std::max(1.0f, static_cast<float>(GetScreenHeight()) - paletteTop - paletteBottomMargin);
    float verticalGap = mobileLayout ? (8.0f * uiScale) : 10.0f;
    int columns = useTwoColumns ? 2 : 1;
    int visibleColorCount = availableEditableColorCount + 1;
    int rows = std::max(1, (visibleColorCount + columns - 1) / columns);
    float buttonWidth = mobileLayout ? (88.0f * uiScale) : 90.0f;
    float horizontalGap = std::max(10.0f * uiScale, 12.0f);

    if (useTwoColumns) {
        float maxWidthForTwoCols = (GetScreenWidth() * 0.42f) - leftMargin;
        float twoColWidth = (maxWidthForTwoCols - horizontalGap) * 0.5f;
        buttonWidth = Clamp(twoColWidth, 64.0f * uiScale, buttonWidth);
    }

    float buttonHeightBySpace = (availableHeight - ((rows - 1) * verticalGap)) / std::max(1, rows);
    float minButtonHeight = mobileLayout ? (24.0f * uiScale) : 36.0f;
    float preferredButtonHeight = mobileLayout ? (36.0f * uiScale) : 48.0f;
    float buttonHeight = Clamp(buttonHeightBySpace, minButtonHeight, preferredButtonHeight);

    for (int i = 0; i < colorCount; i++) {
        if (i > availableEditableColorCount) {
            colorButtons[i].setPosition(0.0f, 0.0f);
            colorButtons[i].setSize(0.0f, 0.0f);
            continue;
        }

        int visibleIndex = i;
        int row = visibleIndex / columns;
        int col = visibleIndex % columns;
        float buttonX = paletteX + col * (buttonWidth + horizontalGap);
        float buttonY = paletteTop + row * (buttonHeight + verticalGap);
        colorButtons[i].setPosition(buttonX, buttonY);
        colorButtons[i].setSize(buttonWidth, buttonHeight);
    }
}
