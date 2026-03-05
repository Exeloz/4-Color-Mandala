#include "paletteScreen.h"
#include "../ui/colorPalette.h"
#include "../ui/colorTileRenderer.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>

namespace {
    constexpr float SLOT_START_X = 48.0f;
    constexpr float SLOT_Y = 112.0f;
    constexpr float SLOT_WIDTH = 68.0f;
    constexpr float SLOT_HEIGHT = 38.0f;
    constexpr float SLOT_GAP = 8.0f;

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
    transitionRequested(false), returnRequested(false), paletteChanged(false) {

    ColorPalette defaultPalette;
    const std::vector<Color> defaultColors = defaultPalette.getColors();
    paletteColors = initialPaletteColors.empty() ? defaultColors : initialPaletteColors;

    if (paletteColors.size() < defaultColors.size()) {
        paletteColors.insert(paletteColors.end(),
                             defaultColors.begin() + paletteColors.size(),
                             defaultColors.end());
    }
}

void PaletteScreen::update(float deltaTime) {
    (void)deltaTime;
    layoutControls();

    if (colorWheelPopup.isVisible()) {
        colorWheelPopup.update();

        int slotIndex = -1;
        Color selectedColor{0, 0, 0, 255};
        if (colorWheelPopup.consumeAccepted(slotIndex, selectedColor)) {
            if (slotIndex > 0 && slotIndex < static_cast<int>(paletteColors.size())) {
                activeSlotIndex = slotIndex;
                Color& slotColor = paletteColors[slotIndex];
                if (slotColor.r != selectedColor.r
                    || slotColor.g != selectedColor.g
                    || slotColor.b != selectedColor.b
                    || slotColor.a != selectedColor.a) {
                    slotColor = selectedColor;
                    paletteChanged = true;
                }
            }
            return;
        }

        if (colorWheelPopup.consumeCancelled(slotIndex)) {
            if (slotIndex > 0 && slotIndex < static_cast<int>(paletteColors.size())) {
                activeSlotIndex = slotIndex;
            }
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
                activeSlotIndex = slotIndex;
                colorWheelPopup.open(slotIndex, paletteColors[slotIndex]);
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

    const char* subtitle = "Slots 1-4: normal + hard  |  Slots 5-10: hard only";
    int subtitleWidth = MeasureText(subtitle, subtitleSize);
    int subtitleX = (GetScreenWidth() - subtitleWidth) / 2;
    DrawText(subtitle, subtitleX, static_cast<int>(78.0f * uiScale), subtitleSize, Colors::DarkSlateGray);

    backButton.draw();
    continueButton.draw();

    for (int i = 1; i < static_cast<int>(paletteColors.size()); i++) {
        Rectangle slot = getPaletteSlotBounds(i);
        ColorTileRenderer::drawColorTile(paletteColors[i], slot, uiScale);

        Color borderColor = (i == activeSlotIndex) ? Colors::Black : Colors::DarkGray;
        float borderWidth = (i == activeSlotIndex) ? 4.0f : 2.0f;
        DrawRectangleLinesEx(slot, borderWidth, borderColor);

        if (i > ColorPalette::NormalEditableColorCount) {
            int hardSize = std::max(10, static_cast<int>(10.0f * uiScale));
            const char* hardTag = "H";
            int hardWidth = MeasureText(hardTag, hardSize);
            int hardX = static_cast<int>(slot.x + slot.width - hardWidth - (6.0f * uiScale));
            int hardY = static_cast<int>(slot.y + (3.0f * uiScale));
            DrawText(hardTag, hardX, hardY, hardSize, Colors::Crimson);
        }
    }

    colorWheelPopup.draw();
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
    if (slotIndex <= 0) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }

    float uiScale = getUiScale();
    float width = SLOT_WIDTH * uiScale;
    float height = SLOT_HEIGHT * uiScale;
    float gap = SLOT_GAP * uiScale;
    float startX = SLOT_START_X * uiScale;
    float y = SLOT_Y * uiScale;

    const int columns = 6;
    const int totalSlots = std::max(0, static_cast<int>(paletteColors.size()) - 1);
    int rows = std::max(1, (totalSlots + columns - 1) / columns);

    float availableWidth = static_cast<float>(GetScreenWidth()) - (2.0f * startX);
    float rowWidthSlots = static_cast<float>(columns) * width + static_cast<float>(columns - 1) * gap;
    if (rowWidthSlots > availableWidth) {
        width = (availableWidth - static_cast<float>(columns - 1) * gap) / static_cast<float>(columns);
        width = std::max(width, 48.0f * uiScale);
    }

    float rowGap = std::max(10.0f * uiScale, SLOT_GAP * uiScale);
    int visibleIndex = slotIndex - 1;
    int row = visibleIndex / columns;
    int col = visibleIndex % columns;

    float x = startX + static_cast<float>(col) * (width + gap);
    float slotY = y + static_cast<float>(row) * (height + rowGap);
    (void)rows;
    return {x, slotY, width, height};
}
