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

    constexpr float TILE_START_X = 90.0f;
    constexpr float TILE_START_Y = 198.0f;
    constexpr float TILE_WIDTH = 78.0f;
    constexpr float TILE_HEIGHT = 36.0f;
    constexpr float TILE_GAP_X = 12.0f;
    constexpr float TILE_GAP_Y = 10.0f;
    constexpr float NAV_BOTTOM_MARGIN = 20.0f;
    constexpr float NAV_RESERVED_HEIGHT = 96.0f;

    bool isMobileLayout() {
        return true;
    }

    float getUiScale() {
        float widthScale = static_cast<float>(GetScreenWidth()) / 860.0f;
        float heightScale = static_cast<float>(GetScreenHeight()) / 420.0f;
        return Clamp(std::min(widthScale, heightScale), 0.75f, 2.4f);
    }

    int getTileColumns(float uiScale) {
        float tileWidth = TILE_WIDTH * uiScale;
        float gapX = TILE_GAP_X * uiScale;
        float startX = TILE_START_X * uiScale;
        float rightMargin = 20.0f * uiScale;

        float availableWidth = std::max(1.0f, static_cast<float>(GetScreenWidth()) - startX - rightMargin);
        int columns = static_cast<int>((availableWidth + gapX) / (tileWidth + gapX));
        return std::max(1, columns);
    }

    int getTileRows(float uiScale) {
        float tileHeight = TILE_HEIGHT * uiScale;
        float gapY = TILE_GAP_Y * uiScale;
        float startY = TILE_START_Y * uiScale;
        float bottomMargin = NAV_RESERVED_HEIGHT * uiScale;

        float availableHeight = std::max(1.0f, static_cast<float>(GetScreenHeight()) - startY - bottomMargin);
        int rows = static_cast<int>((availableHeight + gapY) / (tileHeight + gapY));
        return std::max(1, rows);
    }

    int getTilesPerPage(float uiScale) {
        return getTileColumns(uiScale) * getTileRows(uiScale);
    }

}

PaletteScreen::PaletteScreen(const std::vector<Color>& initialPaletteColors)
    : paletteColors(), availableColors(), activeSlotIndex(1),
      backButton(20, 20, 100, 50, "BACK"), continueButton(640, 20, 140, 50, "COLOR"),
      prevPageButton(560, 200, 100, 40, "PREV"), nextPageButton(680, 200, 100, 40, "NEXT"),
    transitionRequested(false), returnRequested(false), tilePage(0) {

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
    layoutControls();

    float uiScale = getUiScale();
    int tilesPerPage = std::max(1, getTilesPerPage(uiScale));
    int maxPage = std::max(0, (static_cast<int>(availableColors.size()) - 1) / tilesPerPage);
    if (tilePage > maxPage) {
        tilePage = maxPage;
    }

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

    if (prevPageButton.isClicked() && tilePage > 0) {
        tilePage--;
        return;
    }

    if (nextPageButton.isClicked() && tilePage < maxPage) {
        tilePage++;
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

    for (int colorIndex = getTileStartIndex(); colorIndex < getTileEndIndex(); colorIndex++) {
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

    float uiScale = getUiScale();
    int titleSize = static_cast<int>(42.0f * uiScale);
    int subtitleSize = static_cast<int>(24.0f * uiScale);
    int pageSize = static_cast<int>(24.0f * uiScale);

    int titleY = static_cast<int>(25.0f * uiScale);
    const char* title = "Customize Palette";
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GetScreenWidth() - titleWidth) / 2;
    DrawText(title, titleX, titleY, titleSize, Colors::Black);

    const char* subtitle = "Tap a palette slot, then choose a color";
    int subtitleWidth = MeasureText(subtitle, subtitleSize);
    int subtitleX = (GetScreenWidth() - subtitleWidth) / 2;
    DrawText(subtitle, subtitleX, static_cast<int>(78.0f * uiScale), subtitleSize, Colors::DarkSlateGray);

    backButton.draw();
    continueButton.draw();
    prevPageButton.draw();
    nextPageButton.draw();

    int tilesPerPage = std::max(1, getTilesPerPage(uiScale));
    int maxPage = std::max(0, (static_cast<int>(availableColors.size()) - 1) / tilesPerPage);
    std::string pageText = "Page " + std::to_string(tilePage + 1) + " / " + std::to_string(maxPage + 1);
    Rectangle prevBounds = prevPageButton.getBounds();
    Rectangle nextBounds = nextPageButton.getBounds();
    int pageTextWidth = MeasureText(pageText.c_str(), pageSize);
    float pageCenterX = (prevBounds.x + nextBounds.x + nextBounds.width) * 0.5f;
    int pageX = static_cast<int>(pageCenterX - (pageTextWidth * 0.5f));
    int pageY = static_cast<int>(prevBounds.y - pageSize - (6.0f * uiScale));
    DrawText(pageText.c_str(), pageX, pageY, pageSize, Colors::Black);

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

    for (int i = getTileStartIndex(); i < getTileEndIndex(); i++) {
        Rectangle tile = getAvailableColorBounds(i);
        ColorTileRenderer::drawColorTile(availableColors[i], tile, uiScale);
        DrawRectangleLinesEx(tile, 2.0f, Colors::DarkGray);

        bool isSelectedColor = (activeSlotIndex > 0 && activeSlotIndex < static_cast<int>(paletteColors.size()) &&
                                paletteColors[activeSlotIndex].r == availableColors[i].r &&
                                paletteColors[activeSlotIndex].g == availableColors[i].g &&
                                paletteColors[activeSlotIndex].b == availableColors[i].b &&
                                paletteColors[activeSlotIndex].a == availableColors[i].a);

        if (isSelectedColor) {
            DrawRectangleLinesEx(tile, 4.0f, Colors::White);
            DrawRectangleLinesEx({tile.x + 2, tile.y + 2, tile.width - 4, tile.height - 4}, 2.0f, Colors::Black);
        }
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

    float navWidth = mobileLayout ? (120.0f * uiScale) : 100.0f;
    float navHeight = mobileLayout ? (52.0f * uiScale) : 40.0f;
    float navY = static_cast<float>(GetScreenHeight()) - navHeight - (NAV_BOTTOM_MARGIN * uiScale);
    nextPageButton.setPosition(GetScreenWidth() - sideMargin - navWidth, navY);
    nextPageButton.setSize(navWidth, navHeight);
    prevPageButton.setPosition(nextPageButton.getBounds().x - navWidth - (12.0f * uiScale), navY);
    prevPageButton.setSize(navWidth, navHeight);
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

Rectangle PaletteScreen::getAvailableColorBounds(int colorIndex) const {
    float uiScale = getUiScale();
    int tileColumns = std::max(1, getTileColumns(uiScale));
    int pageLocalIndex = colorIndex - getTileStartIndex();
    int col = pageLocalIndex % tileColumns;
    int row = pageLocalIndex / tileColumns;

    float width = TILE_WIDTH * uiScale;
    float height = TILE_HEIGHT * uiScale;
    float gapX = TILE_GAP_X * uiScale;
    float gapY = TILE_GAP_Y * uiScale;
    float startX = TILE_START_X * uiScale;
    float startY = TILE_START_Y * uiScale;

    float x = startX + col * (width + gapX);
    float y = startY + row * (height + gapY);
    return {x, y, width, height};
}

int PaletteScreen::getTileStartIndex() const {
    int tilesPerPage = std::max(1, getTilesPerPage(getUiScale()));
    return tilePage * tilesPerPage;
}

int PaletteScreen::getTileEndIndex() const {
    int tilesPerPage = std::max(1, getTilesPerPage(getUiScale()));
    int endIndex = getTileStartIndex() + tilesPerPage;
    return std::min(static_cast<int>(availableColors.size()), endIndex);
}
