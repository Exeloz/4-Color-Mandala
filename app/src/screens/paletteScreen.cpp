#include "paletteScreen.h"
#include "../ui/colorPalette.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include <algorithm>
#include <string>

namespace {
    constexpr float SLOT_START_X = 90.0f;
    constexpr float SLOT_Y = 120.0f;
    constexpr float SLOT_WIDTH = 120.0f;
    constexpr float SLOT_HEIGHT = 70.0f;
    constexpr float SLOT_GAP = 20.0f;

    constexpr int SWATCH_COLUMNS = 6;
    constexpr int SWATCH_ROWS = 4;
    constexpr float SWATCH_START_X = 90.0f;
    constexpr float SWATCH_START_Y = 260.0f;
    constexpr float SWATCH_WIDTH = 95.0f;
    constexpr float SWATCH_HEIGHT = 50.0f;
    constexpr float SWATCH_GAP_X = 20.0f;
    constexpr float SWATCH_GAP_Y = 15.0f;
}

PaletteScreen::PaletteScreen(const std::vector<Color>& initialPaletteColors)
    : paletteColors(), availableColors(), activeSlotIndex(1),
      backButton(20, 20, 100, 50, "BACK"), continueButton(640, 20, 140, 50, "COLOR"),
      prevPageButton(560, 200, 100, 40, "PREV"), nextPageButton(680, 200, 100, 40, "NEXT"),
      transitionRequested(false), returnRequested(false), swatchPage(0) {

    ColorPalette defaultPalette;
    paletteColors = initialPaletteColors.empty() ? defaultPalette.getColors() : initialPaletteColors;

    availableColors = {
        Colors::None,
        Colors::Red, Colors::DarkRed, Colors::Crimson, Colors::FireBrick, Colors::Coral, Colors::Salmon, Colors::LightCoral, Colors::IndianRed,
        Colors::Orange, Colors::DarkOrange, Colors::OrangeRed, Colors::Tomato, Colors::Tangerine, Colors::PeachPuff,
        Colors::Yellow, Colors::Gold, Colors::LightYellow, Colors::LemonChiffon, Colors::Khaki, Colors::DarkKhaki,
        Colors::Green, Colors::DarkGreen, Colors::ForestGreen, Colors::LimeGreen, Colors::Lime, Colors::SpringGreen, Colors::SeaGreen, Colors::MediumSeaGreen,
        Colors::LightGreen, Colors::PaleGreen, Colors::Chartreuse, Colors::YellowGreen, Colors::OliveDrab, Colors::Olive,
        Colors::Cyan, Colors::Aqua, Colors::DarkCyan, Colors::LightCyan, Colors::Turquoise, Colors::MediumTurquoise, Colors::DarkTurquoise, Colors::Aquamarine, Colors::PaleTurquoise,
        Colors::Blue, Colors::DarkBlue, Colors::MediumBlue, Colors::Navy, Colors::RoyalBlue, Colors::SteelBlue, Colors::DodgerBlue, Colors::DeepSkyBlue,
        Colors::SkyBlue, Colors::LightSkyBlue, Colors::LightBlue, Colors::PowderBlue, Colors::CornflowerBlue, Colors::CadetBlue,
        Colors::Purple, Colors::DarkMagenta, Colors::DarkViolet, Colors::DarkOrchid, Colors::Indigo, Colors::BlueViolet, Colors::MediumPurple, Colors::MediumOrchid,
        Colors::Orchid, Colors::Violet, Colors::Plum, Colors::Thistle, Colors::Lavender,
        Colors::Magenta, Colors::Fuchsia, Colors::DeepPink, Colors::HotPink, Colors::Pink, Colors::LightPink, Colors::PaleVioletRed, Colors::MediumVioletRed,
        Colors::Brown, Colors::SaddleBrown, Colors::Sienna, Colors::Chocolate, Colors::Peru, Colors::SandyBrown, Colors::BurlyWood, Colors::Tan, Colors::RosyBrown, Colors::Wheat, Colors::Beige,
        Colors::White, Colors::Snow, Colors::Ivory, Colors::Honeydew, Colors::MintCream, Colors::Azure, Colors::AliceBlue, Colors::GhostWhite, Colors::WhiteSmoke,
        Colors::Seashell, Colors::OldLace, Colors::FloralWhite, Colors::AntiqueWhite, Colors::Linen, Colors::LavenderBlush, Colors::MistyRose,
        Colors::Gray, Colors::DarkGray, Colors::Silver, Colors::LightGray, Colors::Gainsboro, Colors::DimGray, Colors::SlateGray, Colors::LightSlateGray, Colors::DarkSlateGray,
        Colors::Black
    };
}

void PaletteScreen::update(float deltaTime) {
    backButton.update();
    continueButton.update();
    prevPageButton.update();
    nextPageButton.update();

    if (backButton.isClicked()) {
        returnRequested = true;
        return;
    }

    if (continueButton.isClicked()) {
        transitionRequested = true;
        return;
    }

    if (prevPageButton.isClicked() && swatchPage > 0) {
        swatchPage--;
        return;
    }

    int maxPage = std::max(0, (static_cast<int>(availableColors.size()) - 1) / (SWATCH_COLUMNS * SWATCH_ROWS));
    if (nextPageButton.isClicked() && swatchPage < maxPage) {
        swatchPage++;
        return;
    }

    if (!Input::IsPointerPressed()) {
        return;
    }

    Vector2 pointer = Input::GetPointerPosition();

    for (int slotIndex = 1; slotIndex < static_cast<int>(paletteColors.size()); slotIndex++) {
        if (CheckCollisionPointRec(pointer, getPaletteSlotBounds(slotIndex))) {
            activeSlotIndex = slotIndex;
            return;
        }
    }

    for (int colorIndex = getSwatchStartIndex(); colorIndex < getSwatchEndIndex(); colorIndex++) {
        if (CheckCollisionPointRec(pointer, getAvailableColorBounds(colorIndex))) {
            if (activeSlotIndex > 0 && activeSlotIndex < static_cast<int>(paletteColors.size())) {
                paletteColors[activeSlotIndex] = availableColors[colorIndex];
            }
            return;
        }
    }
}

void PaletteScreen::draw() {
    ClearBackground(Colors::Gainsboro);

    DrawText("Customize Palette", 205, 25, 42, Colors::Black);
    DrawText("Tap a palette slot, then choose a color", 195, 78, 24, Colors::DarkSlateGray);

    backButton.draw();
    continueButton.draw();
    prevPageButton.draw();
    nextPageButton.draw();

    int maxPage = std::max(0, (static_cast<int>(availableColors.size()) - 1) / (SWATCH_COLUMNS * SWATCH_ROWS));
    std::string pageText = "Page " + std::to_string(swatchPage + 1) + " / " + std::to_string(maxPage + 1);
    DrawText(pageText.c_str(), 90, 208, 24, Colors::Black);

    for (int i = 0; i < static_cast<int>(paletteColors.size()); i++) {
        Rectangle slot = getPaletteSlotBounds(i);
        DrawRectangleRec(slot, paletteColors[i]);

        Color borderColor = (i == activeSlotIndex) ? Colors::Black : Colors::DarkGray;
        float borderWidth = (i == activeSlotIndex) ? 4.0f : 2.0f;
        DrawRectangleLinesEx(slot, borderWidth, borderColor);

        if (i == 0) {
            DrawText("LOCK", static_cast<int>(slot.x + 35), static_cast<int>(slot.y + 24), 20, Colors::Black);
        }
    }

    for (int i = getSwatchStartIndex(); i < getSwatchEndIndex(); i++) {
        Rectangle swatch = getAvailableColorBounds(i);
        DrawRectangleRec(swatch, availableColors[i]);
        DrawRectangleLinesEx(swatch, 2.0f, Colors::DarkGray);

        bool isSelectedColor = (activeSlotIndex > 0 && activeSlotIndex < static_cast<int>(paletteColors.size()) &&
                                paletteColors[activeSlotIndex].r == availableColors[i].r &&
                                paletteColors[activeSlotIndex].g == availableColors[i].g &&
                                paletteColors[activeSlotIndex].b == availableColors[i].b &&
                                paletteColors[activeSlotIndex].a == availableColors[i].a);

        if (isSelectedColor) {
            DrawRectangleLinesEx(swatch, 4.0f, Colors::White);
            DrawRectangleLinesEx({swatch.x + 2, swatch.y + 2, swatch.width - 4, swatch.height - 4}, 2.0f, Colors::Black);
        }
    }
}

bool PaletteScreen::shouldTransitionToColoring() const {
    return transitionRequested;
}

bool PaletteScreen::shouldReturnToSelection() const {
    return returnRequested;
}

const std::vector<Color>& PaletteScreen::getCustomizedColors() const {
    return paletteColors;
}

Rectangle PaletteScreen::getPaletteSlotBounds(int slotIndex) const {
    float x = SLOT_START_X + slotIndex * (SLOT_WIDTH + SLOT_GAP);
    return {x, SLOT_Y, SLOT_WIDTH, SLOT_HEIGHT};
}

Rectangle PaletteScreen::getAvailableColorBounds(int colorIndex) const {
    int pageLocalIndex = colorIndex - getSwatchStartIndex();
    int col = pageLocalIndex % SWATCH_COLUMNS;
    int row = pageLocalIndex / SWATCH_COLUMNS;

    float x = SWATCH_START_X + col * (SWATCH_WIDTH + SWATCH_GAP_X);
    float y = SWATCH_START_Y + row * (SWATCH_HEIGHT + SWATCH_GAP_Y);
    return {x, y, SWATCH_WIDTH, SWATCH_HEIGHT};
}

int PaletteScreen::getSwatchStartIndex() const {
    int swatchesPerPage = SWATCH_COLUMNS * SWATCH_ROWS;
    return swatchPage * swatchesPerPage;
}

int PaletteScreen::getSwatchEndIndex() const {
    int swatchesPerPage = SWATCH_COLUMNS * SWATCH_ROWS;
    int endIndex = getSwatchStartIndex() + swatchesPerPage;
    return std::min(static_cast<int>(availableColors.size()), endIndex);
}
