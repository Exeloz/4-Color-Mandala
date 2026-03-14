#include "coloringScreen.h"

#include "../ui/colorTileRenderer.h"
#include "../ui/colors.h"

#include "raymath.h"

#include <algorithm>

namespace {
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

void ColoringScreen::drawColorPalette() {
    if (colorButtons.empty()) {
        return;
    }

    int availableEditableColorCount = getAvailableEditableColorCount(*mandala, colorPalette.getColorCount());
    if (availableEditableColorCount <= 0) {
        return;
    }

    float uiScale = getUiScale();
    Rectangle firstButton = colorButtons[0].getBounds();
    int labelSize = static_cast<int>(16.0f * uiScale);
    int labelX = static_cast<int>(firstButton.x);
    int labelY = static_cast<int>(firstButton.y - labelSize - (10.0f * uiScale));
    DrawText("Colors:", labelX, labelY, labelSize, Colors::Black);

    for (int colorIndex = 0; colorIndex <= availableEditableColorCount; colorIndex++) {
        Color color = colorPalette.getColor(colorIndex);
        Rectangle bounds = colorButtons[colorIndex].getBounds();
        ColorTileRenderer::drawColorTile(color, bounds, uiScale, FillPattern(), colorIndex == 0);

        if (colorIndex == colorPalette.getSelectedColorIndex()) {
            DrawRectangleLinesEx(bounds, 7.0f, Colors::Black);
        } else {
            DrawRectangleLinesEx(bounds, 2.0f, Colors::Gray);
        }
    }
}

void ColoringScreen::drawBadgePopup(const StatusBadge& badge, Rectangle badgeRect) const {
    float uiScale = getUiScale();
    int titleSize  = std::max(14, static_cast<int>(14.0f * uiScale));
    int bodySize   = std::max(12, static_cast<int>(12.0f * uiScale));
    const int padding = static_cast<int>(10.0f * uiScale);
    const float lineSpacing = bodySize * 1.4f;

    std::vector<std::string> lines;
    std::string rem = badge.description;
    std::string::size_type pos;
    while ((pos = rem.find('\n')) != std::string::npos) {
        lines.push_back(rem.substr(0, pos));
        rem = rem.substr(pos + 1);
    }
    if (!rem.empty()) lines.push_back(rem);

    int maxTextWidth = MeasureText(badge.label.c_str(), titleSize);
    for (const auto& line : lines) {
        maxTextWidth = std::max(maxTextWidth, MeasureText(line.c_str(), bodySize));
    }

    float popupW = static_cast<float>(maxTextWidth) + 2.0f * padding;
    float popupH = padding + titleSize + (padding / 2)
                   + static_cast<float>(lines.size()) * lineSpacing
                   + padding;

    float popupX = badgeRect.x;
    float popupY = badgeRect.y + badgeRect.height + 6.0f * uiScale;
    if (popupX + popupW > GetScreenWidth() - 8.0f) {
        popupX = GetScreenWidth() - popupW - 8.0f;
    }

    DrawRectangleRec({popupX, popupY, popupW, popupH}, {30, 10, 50, 230});
    DrawRectangleLinesEx({popupX, popupY, popupW, popupH}, 2.0f, Colors::MediumPurple);

    DrawText(badge.label.c_str(), static_cast<int>(popupX + padding),
             static_cast<int>(popupY + padding), titleSize, Colors::Violet);

    float lineY = popupY + padding + titleSize + (padding / 2);
    for (const auto& line : lines) {
        DrawText(line.c_str(), static_cast<int>(popupX + padding),
                 static_cast<int>(lineY), bodySize, Colors::White);
        lineY += lineSpacing;
    }
}

bool ColoringScreen::isPointerOverUi(Vector2 screenPos) const {
    return interactionManager.isPointerOverUi(
        screenPos,
        inspector.isAnalysisMode(),
        backButton,
        undoButton,
        validateButton,
        analysisButton,
        analysisCloseButton,
        analysisClearButton,
        colorButtons);
}
