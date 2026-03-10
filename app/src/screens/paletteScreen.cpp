#include "paletteScreen.h"
#include "../ui/colorPalette.h"
#include "../ui/colorTileRenderer.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>

namespace {
    constexpr float SLOT_PANEL_MARGIN_X = 28.0f;
    constexpr float SLOT_PANEL_TOP = 102.0f;
    constexpr float SLOT_PANEL_BOTTOM_MARGIN = 188.0f;

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
    int subtitleSize = static_cast<int>(22.0f * uiScale);
    int titleY = static_cast<int>(25.0f * uiScale);
    const char* title = "Color Wheel";
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GetScreenWidth() - titleWidth) / 2;
    DrawText(title, titleX, titleY, titleSize, Colors::Black);

    const char* subtitle = "Shared colors first. HARD-tagged colors are used only in hard mode.";
    int subtitleWidth = MeasureText(subtitle, subtitleSize);
    int subtitleX = (GetScreenWidth() - subtitleWidth) / 2;
    DrawText(subtitle, subtitleX, static_cast<int>(78.0f * uiScale), subtitleSize, Colors::DarkSlateGray);

    Rectangle slotsPanel = {
        28.0f * uiScale,
        102.0f * uiScale,
        static_cast<float>(GetScreenWidth()) - (56.0f * uiScale),
        std::max(90.0f * uiScale, static_cast<float>(GetScreenHeight()) - (188.0f * uiScale))
    };
    DrawRectangleRounded(slotsPanel, 0.06f, 10, Fade(Colors::WhiteSmoke, 0.80f));
    DrawRectangleRoundedLinesEx(slotsPanel, 0.06f, 10, 2.0f, Fade(Colors::SlateGray, 0.5f));

    backButton.draw();
    continueButton.draw();

    Vector2 pointer = Input::GetPointerPosition();

    for (int i = 1; i < static_cast<int>(paletteColors.size()); i++) {
        Rectangle slot = getPaletteSlotBounds(i);
        bool isHovered = CheckCollisionPointRec(pointer, slot);
        bool isActive = (i == activeSlotIndex);

        Rectangle card = {
            slot.x - (5.0f * uiScale),
            slot.y - (5.0f * uiScale),
            slot.width + (10.0f * uiScale),
            slot.height + (10.0f * uiScale)
        };

        DrawRectangleRounded(card, 0.2f, 8, Fade(Colors::White, isHovered ? 0.98f : 0.88f));

        ColorTileRenderer::drawColorTile(paletteColors[i], slot, uiScale);

        Color frameColor = isActive ? Colors::DarkBlue : (isHovered ? Colors::RoyalBlue : Colors::DarkGray);
        float frameWidth = isActive ? 4.0f : (isHovered ? 2.8f : 2.0f);
        DrawRectangleRoundedLinesEx(card, 0.2f, 8, frameWidth, frameColor);

        if (i > ColorPalette::NormalEditableColorCount) {
            int hardSize = std::max(10, static_cast<int>(10.0f * uiScale));
            const char* hardTag = "HARD";
            int hardWidth = MeasureText(hardTag, hardSize);
            float badgePaddingX = 4.0f * uiScale;
            float badgePaddingY = 2.0f * uiScale;
            float badgeW = hardWidth + (badgePaddingX * 2.0f);
            float badgeH = hardSize + (badgePaddingY * 2.0f);
            float badgeX = slot.x + slot.width - badgeW - (4.0f * uiScale);
            float badgeY = slot.y + (2.0f * uiScale);
            Rectangle badge = {badgeX, badgeY, badgeW, badgeH};
            DrawRectangleRounded(badge, 0.35f, 6, Fade(Colors::Crimson, 0.92f));
            DrawText(hardTag,
                     static_cast<int>(badgeX + badgePaddingX),
                     static_cast<int>(badgeY + badgePaddingY),
                     hardSize,
                     Colors::White);
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
    Rectangle slotsPanel = {
        SLOT_PANEL_MARGIN_X * uiScale,
        SLOT_PANEL_TOP * uiScale,
        static_cast<float>(GetScreenWidth()) - (2.0f * SLOT_PANEL_MARGIN_X * uiScale),
        std::max(90.0f * uiScale, static_cast<float>(GetScreenHeight()) - (SLOT_PANEL_BOTTOM_MARGIN * uiScale))
    };

    const int totalSlots = std::max(0, static_cast<int>(paletteColors.size()) - 1);
    const bool landscape = GetScreenWidth() >= GetScreenHeight();
    const int columns = landscape ? 5 : 4;
    const int rows = std::max(1, (totalSlots + columns - 1) / columns);

    float gapX = std::max(10.0f * uiScale, slotsPanel.width * 0.022f);
    float gapY = std::max(10.0f * uiScale, slotsPanel.height * 0.08f);
    float width = (slotsPanel.width - ((columns + 1) * gapX)) / static_cast<float>(columns);
    float height = (slotsPanel.height - ((rows + 1) * gapY)) / static_cast<float>(rows);
    width = std::max(width, 46.0f * uiScale);
    height = std::max(height, 34.0f * uiScale);

    int visibleIndex = slotIndex - 1;
    int row = visibleIndex / columns;
    int col = visibleIndex % columns;

    float x = slotsPanel.x + gapX + static_cast<float>(col) * (width + gapX);
    float slotY = slotsPanel.y + gapY + static_cast<float>(row) * (height + gapY);
    return {x, slotY, width, height};
}
