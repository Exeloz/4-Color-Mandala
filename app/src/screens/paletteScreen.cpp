#include "paletteScreen.h"
#include "../ui/colorPalette.h"
#include "../ui/colorTileRenderer.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>
#include <string>

namespace {
    constexpr float SLOT_START_X = 86.0f;
    constexpr float SLOT_Y = 112.0f;
    constexpr float SLOT_WIDTH = 76.0f;
    constexpr float SLOT_HEIGHT = 38.0f;
    constexpr float SLOT_GAP = 8.0f;

    constexpr float POPUP_WIDTH_RATIO = 0.86f;
    constexpr float POPUP_HEIGHT_RATIO = 0.82f;

    bool isMobileLayout() {
        return true;
    }

    float getUiScale() {
        float widthScale = static_cast<float>(GetScreenWidth()) / 860.0f;
        float heightScale = static_cast<float>(GetScreenHeight()) / 420.0f;
        return Clamp(std::min(widthScale, heightScale), 0.75f, 2.4f);
    }

}

PaletteScreen::PaletteScreen(const std::vector<Color>& initialPaletteColors)
    : paletteColors(), activeSlotIndex(-1),
      backButton(20, 20, 100, 50, "BACK"), continueButton(640, 20, 140, 50, "COLOR"),
            wheelAcceptButton(0, 0, 130, 52, "Apply"), wheelCancelButton(0, 0, 130, 52, "Cancel"),
            transitionRequested(false), returnRequested(false), paletteChanged(false),
            wheelPopupVisible(false), wheelPopupSlotIndex(-1) {

    ColorPalette defaultPalette;
    paletteColors = initialPaletteColors.empty() ? defaultPalette.getColors() : initialPaletteColors;
}

void PaletteScreen::update(float deltaTime) {
    (void)deltaTime;
    layoutControls();

    if (wheelPopupVisible) {
        layoutWheelPopup();
        wheelCancelButton.update();
        wheelAcceptButton.update();
        colorWheelPicker.update();

        if (wheelCancelButton.isClicked()) {
            closeWheelPopup(false);
            return;
        }

        if (wheelAcceptButton.isClicked()) {
            closeWheelPopup(true);
            return;
        }

        return;
    }

    backButton.update();
    continueButton.update();

    if (backButton.isClicked()) {
        returnRequested = true;
        return;
    }

    if (continueButton.isClicked()) {
        transitionRequested = true;
        return;
    }

    if (Input::IsPointerPressed()) {
        Vector2 pointer = Input::GetPointerPosition();

        for (int slotIndex = 1; slotIndex < static_cast<int>(paletteColors.size()); slotIndex++) {
            if (CheckCollisionPointRec(pointer, getPaletteSlotBounds(slotIndex))) {
                openWheelPopupForSlot(slotIndex);
                return;
            }
        }
    }
}

void PaletteScreen::draw() {
    ClearBackground(Colors::Gainsboro);

    float uiScale = getUiScale();
    int titleSize = static_cast<int>(42.0f * uiScale);
    int subtitleSize = static_cast<int>(24.0f * uiScale);
    int titleY = static_cast<int>(25.0f * uiScale);
    const char* title = "Color Wheel";
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GetScreenWidth() - titleWidth) / 2;
    DrawText(title, titleX, titleY, titleSize, Colors::Black);

    const char* subtitle = "Tap a palette slot, then pick from the wheel";
    int subtitleWidth = MeasureText(subtitle, subtitleSize);
    int subtitleX = (GetScreenWidth() - subtitleWidth) / 2;
    DrawText(subtitle, subtitleX, static_cast<int>(78.0f * uiScale), subtitleSize, Colors::DarkSlateGray);

    backButton.draw();
    continueButton.draw();

    for (int i = 0; i < static_cast<int>(paletteColors.size()); i++) {
        Rectangle slot = getPaletteSlotBounds(i);
        ColorTileRenderer::drawColorTile(paletteColors[i], slot, uiScale);

        Color borderColor = (i == activeSlotIndex) ? Colors::Black : Colors::DarkGray;
        float borderWidth = (i == activeSlotIndex) ? 4.0f : 2.0f;
        DrawRectangleLinesEx(slot, borderWidth, borderColor);

        if (i == 0) {
            int lockSize = std::min(static_cast<int>(16.0f * uiScale), static_cast<int>(slot.height * 0.45f));
            int lockWidth = MeasureText("LOCK", lockSize);
            int lockX = static_cast<int>(slot.x + (slot.width - lockWidth) * 0.5f);
            int lockY = static_cast<int>(slot.y + (slot.height - lockSize) * 0.5f);
            DrawText("LOCK", lockX, lockY, lockSize, Colors::Black);
        }
    }

    if (wheelPopupVisible) {
        Rectangle popupBounds = getWheelPopupBounds();
        Rectangle wheelBounds = getWheelBoundsInPopup(popupBounds);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Colors::Black, 0.55f));
        DrawRectangleRounded(popupBounds, 0.08f, 14, Colors::WhiteSmoke);
        DrawRectangleRoundedLinesEx(popupBounds, 0.08f, 14, 3.0f, Colors::DarkBlue);

        int modalTitleSize = std::max(24, static_cast<int>(popupBounds.height * 0.07f));
        std::string modalTitle = "Edit slot " + std::to_string(wheelPopupSlotIndex);
        int modalTitleWidth = MeasureText(modalTitle.c_str(), modalTitleSize);
        int modalTitleX = static_cast<int>(popupBounds.x + (popupBounds.width - modalTitleWidth) * 0.5f);
        int modalTitleY = static_cast<int>(popupBounds.y + popupBounds.height * 0.06f);
        DrawText(modalTitle.c_str(), modalTitleX, modalTitleY, modalTitleSize, Colors::Black);

        Color previewColor = colorWheelPicker.getSelectedColor();
        float previewSize = std::max(34.0f, popupBounds.height * 0.11f);
        float previewX = popupBounds.x + popupBounds.width * 0.025f;
        float previewY = wheelBounds.y + (wheelBounds.height - previewSize) * 0.5f;
        Rectangle previewSwatch = {previewX, previewY, previewSize, previewSize};
        DrawRectangleRec(previewSwatch, previewColor);
        DrawRectangleLinesEx(previewSwatch, 2.0f, Colors::Black);

        int previewTextSize = std::max(15, static_cast<int>(popupBounds.height * 0.035f));
        const char* selectedText = "Preview";
        DrawText(selectedText,
                 static_cast<int>(previewSwatch.x + (previewSwatch.width - MeasureText(selectedText, previewTextSize)) * 0.5f),
                 static_cast<int>(previewSwatch.y + previewSwatch.height + 6.0f),
                 previewTextSize,
                 Colors::Black);

        colorWheelPicker.draw(Colors::WhiteSmoke);
        wheelCancelButton.draw();
        wheelAcceptButton.draw();
    }
}

void PaletteScreen::layoutControls() {
    float uiScale = getUiScale();
    bool mobileLayout = isMobileLayout();

    float sideMargin = 20.0f * uiScale;
    float topMargin = 20.0f * uiScale;
    float topButtonHeight = mobileLayout ? (58.0f * uiScale) : 50.0f;

    backButton.setPosition(sideMargin, topMargin);
    backButton.setSize(mobileLayout ? (136.0f * uiScale) : 100.0f, topButtonHeight);

    float continueWidth = mobileLayout ? (170.0f * uiScale) : 140.0f;
    continueButton.setPosition(GetScreenWidth() - sideMargin - continueWidth, topMargin);
    continueButton.setSize(continueWidth, topButtonHeight);
}

void PaletteScreen::layoutWheelPopup() {
    Rectangle popupBounds = getWheelPopupBounds();
    colorWheelPicker.setBounds(getWheelBoundsInPopup(popupBounds));

    float buttonWidth = std::max(130.0f, popupBounds.width * 0.24f);
    float buttonHeight = std::max(48.0f, popupBounds.height * 0.13f);
    float buttonY = popupBounds.y + popupBounds.height - buttonHeight - (popupBounds.height * 0.06f);
    float sidePadding = popupBounds.width * 0.08f;

    wheelCancelButton.setPosition(popupBounds.x + sidePadding, buttonY);
    wheelCancelButton.setSize(buttonWidth, buttonHeight);

    wheelAcceptButton.setPosition(popupBounds.x + popupBounds.width - sidePadding - buttonWidth, buttonY);
    wheelAcceptButton.setSize(buttonWidth, buttonHeight);
}

void PaletteScreen::openWheelPopupForSlot(int slotIndex) {
    if (slotIndex <= 0 || slotIndex >= static_cast<int>(paletteColors.size())) {
        return;
    }

    activeSlotIndex = slotIndex;
    wheelPopupSlotIndex = slotIndex;
    colorWheelPicker.setColor(paletteColors[slotIndex]);
    wheelPopupVisible = true;
}

void PaletteScreen::closeWheelPopup(bool applyChanges) {
    if (!wheelPopupVisible || wheelPopupSlotIndex <= 0 || wheelPopupSlotIndex >= static_cast<int>(paletteColors.size())) {
        wheelPopupVisible = false;
        wheelPopupSlotIndex = -1;
        return;
    }

    if (applyChanges) {
        Color selectedColor = colorWheelPicker.getSelectedColor();
        Color& slotColor = paletteColors[wheelPopupSlotIndex];
        if (slotColor.r != selectedColor.r
            || slotColor.g != selectedColor.g
            || slotColor.b != selectedColor.b
            || slotColor.a != selectedColor.a) {
            slotColor = selectedColor;
            paletteChanged = true;
        }
    }

    wheelPopupVisible = false;
    wheelPopupSlotIndex = -1;
}

bool PaletteScreen::shouldTransitionToColoring() const {
    return transitionRequested;
}

bool PaletteScreen::shouldReturnToSelection() const {
    return returnRequested;
}

bool PaletteScreen::consumePaletteChanged() {
    if (!paletteChanged) {
        return false;
    }

    paletteChanged = false;
    return true;
}

const std::vector<Color>& PaletteScreen::getCustomizedColors() const {
    return paletteColors;
}

Rectangle PaletteScreen::getPaletteSlotBounds(int slotIndex) const {
    float uiScale = getUiScale();
    float width = SLOT_WIDTH * uiScale;
    float height = SLOT_HEIGHT * uiScale;
    float gap = SLOT_GAP * uiScale;
    float startX = SLOT_START_X * uiScale;
    float y = SLOT_Y * uiScale;

    float totalWidth = (static_cast<float>(paletteColors.size()) * width) +
                       (static_cast<float>(paletteColors.size() - 1) * gap);
    if (totalWidth > GetScreenWidth() - (2.0f * startX)) {
        width = ((GetScreenWidth() - (2.0f * startX)) - (static_cast<float>(paletteColors.size() - 1) * gap)) /
                static_cast<float>(paletteColors.size());
        width = std::max(width, 60.0f * uiScale);
    }

    float x = startX + slotIndex * (width + gap);
    return {x, y, width, height};
}

Rectangle PaletteScreen::getWheelPopupBounds() const {
    float width = std::min(720.0f, GetScreenWidth() * POPUP_WIDTH_RATIO);
    float height = std::min(720.0f, GetScreenHeight() * POPUP_HEIGHT_RATIO);
    return {
        (GetScreenWidth() - width) * 0.5f,
        (GetScreenHeight() - height) * 0.5f,
        width,
        height
    };
}

Rectangle PaletteScreen::getWheelBoundsInPopup(const Rectangle& popupBounds) const {
    float horizontalPadding = popupBounds.width * 0.08f;
    float topPadding = popupBounds.height * 0.18f;
    float bottomPadding = popupBounds.height * 0.24f;
    float centerShift = popupBounds.width * 0.05f;
    return {
        popupBounds.x + horizontalPadding + centerShift,
        popupBounds.y + topPadding,
        popupBounds.width - (2.0f * horizontalPadding),
        popupBounds.height - topPadding - bottomPadding
    };
}
