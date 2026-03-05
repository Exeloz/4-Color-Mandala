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

    constexpr float WHEEL_TOP_Y = 184.0f;
    constexpr float WHEEL_BOTTOM_MARGIN = 24.0f;
    constexpr float PREVIEW_WIDTH = 200.0f;
    constexpr float PREVIEW_HEIGHT = 70.0f;

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
    : paletteColors(), activeSlotIndex(1),
      backButton(20, 20, 100, 50, "BACK"), continueButton(640, 20, 140, 50, "COLOR"),
      transitionRequested(false), returnRequested(false), paletteChanged(false) {

    ColorPalette defaultPalette;
    paletteColors = initialPaletteColors.empty() ? defaultPalette.getColors() : initialPaletteColors;
    if (activeSlotIndex > 0 && activeSlotIndex < static_cast<int>(paletteColors.size())) {
        colorWheelPicker.setColor(paletteColors[activeSlotIndex]);
    }
}

void PaletteScreen::update(float deltaTime) {
    (void)deltaTime;
    layoutControls();

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
                colorWheelPicker.setColor(paletteColors[activeSlotIndex]);
                return;
            }
        }
    }

    colorWheelPicker.update();

    if (activeSlotIndex > 0 && activeSlotIndex < static_cast<int>(paletteColors.size())) {
        Color selectedColor = colorWheelPicker.getSelectedColor();
        Color& slotColor = paletteColors[activeSlotIndex];
        if (slotColor.r != selectedColor.r
            || slotColor.g != selectedColor.g
            || slotColor.b != selectedColor.b
            || slotColor.a != selectedColor.a) {
            slotColor = selectedColor;
            paletteChanged = true;
        }
    }
}

void PaletteScreen::draw() {
    ClearBackground(Colors::Gainsboro);

    float uiScale = getUiScale();
    int titleSize = static_cast<int>(42.0f * uiScale);
    int subtitleSize = static_cast<int>(24.0f * uiScale);
    int infoSize = static_cast<int>(22.0f * uiScale);

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

    colorWheelPicker.draw(Colors::Gainsboro);

    Rectangle previewBounds = getPreviewBounds();
    DrawRectangleRounded(previewBounds, 0.18f, 12, Colors::WhiteSmoke);
    DrawRectangleLinesEx(previewBounds, 2.0f, Colors::DarkGray);

    Color selectedColor = colorWheelPicker.getSelectedColor();
    Rectangle swatch = {
        previewBounds.x + 12.0f * uiScale,
        previewBounds.y + 12.0f * uiScale,
        previewBounds.height - 24.0f * uiScale,
        previewBounds.height - 24.0f * uiScale
    };
    DrawRectangleRec(swatch, selectedColor);
    DrawRectangleLinesEx(swatch, 2.0f, Colors::Black);

    std::string slotText = "Slot " + std::to_string(activeSlotIndex);
    DrawText(slotText.c_str(),
             static_cast<int>(swatch.x + swatch.width + (12.0f * uiScale)),
             static_cast<int>(previewBounds.y + 14.0f * uiScale),
             infoSize,
             Colors::Black);

    DrawText("Selected exact color",
             static_cast<int>(swatch.x + swatch.width + (12.0f * uiScale)),
             static_cast<int>(previewBounds.y + 14.0f * uiScale + infoSize + (8.0f * uiScale)),
             static_cast<int>(18.0f * uiScale),
             Colors::DarkSlateGray);
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

    colorWheelPicker.setBounds(getColorWheelBounds());
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

Rectangle PaletteScreen::getColorWheelBounds() const {
    float uiScale = getUiScale();
    float top = WHEEL_TOP_Y * uiScale;
    float bottomMargin = WHEEL_BOTTOM_MARGIN * uiScale;

    float maxDiameter = std::min(
        static_cast<float>(GetScreenWidth()) - (80.0f * uiScale),
        static_cast<float>(GetScreenHeight()) - top - bottomMargin - (PREVIEW_HEIGHT * uiScale) - (12.0f * uiScale)
    );
    float diameter = std::max(140.0f * uiScale, maxDiameter);

    float x = (GetScreenWidth() - diameter) * 0.5f;
    return {x, top, diameter, diameter};
}

Rectangle PaletteScreen::getPreviewBounds() const {
    float uiScale = getUiScale();
    Rectangle wheel = getColorWheelBounds();
    float width = PREVIEW_WIDTH * uiScale;
    float height = PREVIEW_HEIGHT * uiScale;
    float x = (GetScreenWidth() - width) * 0.5f;
    float y = wheel.y + wheel.height + (12.0f * uiScale);
    return {x, y, width, height};
}
