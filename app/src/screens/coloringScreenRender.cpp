#include "coloringScreen.h"

#include "../ui/colors.h"
#include "raymath.h"

#include <algorithm>
#include <cfloat>
#include <string>

namespace {
constexpr float MIN_ZOOM = 0.02f;
constexpr float MAX_ZOOM = 4.0f;
constexpr float CAMERA_FIT_MARGIN = 24.0f;

float getUiScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / 860.0f;
    float heightScale = static_cast<float>(GetScreenHeight()) / 420.0f;
    return Clamp(std::min(widthScale, heightScale), 0.75f, 2.4f);
}

std::string fitTextWithEllipsis(const std::string& text, int textSize, int maxWidth) {
    const std::string ellipsis = "...";
    if (MeasureText(text.c_str(), textSize) <= maxWidth) {
        return text;
    }

    std::string trimmed = text;
    while (!trimmed.empty()) {
        trimmed.pop_back();
        std::string candidate = trimmed + ellipsis;
        if (MeasureText(candidate.c_str(), textSize) <= maxWidth) {
            return candidate;
        }
    }

    return ellipsis;
}

void appendStatusBadges(std::vector<StatusBadge>& allBadges,
                        const Mandala& mandala,
                        bool readOnlyMode,
                        const std::vector<StatusBadge>& ruleBadges) {
    allBadges.clear();

    if (mandala.isHardMode()) {
        allBadges.push_back({"HARD",
            "Hard mode: regions are adjacent\neven when they only touch at a tip.",
            Colors::Crimson, Colors::DarkRed, Colors::White, Colors::Black});

        if (mandala.getMinimumColorCount() > 0) {
            const std::string minLabel = "MIN " + std::to_string(mandala.getMinimumColorCount());
            allBadges.push_back({minLabel,
                "Minimum colors: this mandala\nrequires at least "
                    + std::to_string(mandala.getMinimumColorCount()) + " colors.",
                Colors::DarkBlue, Colors::Navy, Colors::White, Colors::Black});
        }
    }

    if (readOnlyMode) {
        allBadges.push_back({"DONE",
            "Completed! You can still\nexplore your solution.",
            Colors::Gold, Colors::DarkKhaki, Colors::Black, Colors::Black});
    }

    for (const auto& badge : ruleBadges) {
        allBadges.push_back(badge);
    }
}
}

void ColoringScreen::draw() {
    ClearBackground(Colors::LightBlue);

    const float uiScale = getUiScale();
    const Rectangle backBounds = backButton.getBounds();
    const int titleFont = static_cast<int>(30.0f * uiScale);
    const int titleX = static_cast<int>(backBounds.x + backBounds.width + (16.0f * uiScale));
    const int titleY = static_cast<int>(20.0f * uiScale);

    Rectangle actionBounds = inspector.isAnalysisMode() ? analysisCloseButton.getBounds() : analysisButton.getBounds();
    int titleMaxWidth = static_cast<int>(actionBounds.x - titleX - (12.0f * uiScale));
    titleMaxWidth = std::max(titleMaxWidth, static_cast<int>(140.0f * uiScale));

    const std::string titleText = fitTextWithEllipsis(mandala->getName(), titleFont, titleMaxWidth);
    DrawText(titleText.c_str(), titleX, titleY, titleFont, Colors::Black);

    float statusBadgeX = static_cast<float>(titleX + MeasureText(titleText.c_str(), titleFont)
                                            + static_cast<int>(10.0f * uiScale));
    float statusBadgeY = static_cast<float>(titleY + static_cast<int>(6.0f * uiScale));

    appendStatusBadges(allBadges, *mandala, readOnlyMode, ruleBadges);

    badgeRects.clear();
    const int labelSize = std::max(12, static_cast<int>(12.0f * uiScale));
    const float badgeHeight = 24.0f * uiScale;
    for (int i = 0; i < static_cast<int>(allBadges.size()); i++) {
        const StatusBadge& badge = allBadges[i];
        if (badge.label.empty()) {
            continue;
        }

        float badgeWidth = static_cast<float>(MeasureText(badge.label.c_str(), labelSize)) + (18.0f * uiScale);
        Rectangle bounds = {statusBadgeX, statusBadgeY, badgeWidth, badgeHeight};
        badgeRects.push_back(bounds);

        bool isOpen = (badgePopupIndex == i) && !badge.description.empty();
        Color background = isOpen ? badge.bgColorOpen : badge.bgColor;
        Color border = isOpen ? Colors::White : badge.borderColor;
        DrawRectangleRec(bounds, background);
        DrawRectangleLinesEx(bounds, 2.0f, border);

        int textX = static_cast<int>(bounds.x + (badgeWidth - MeasureText(badge.label.c_str(), labelSize)) * 0.5f);
        int textY = static_cast<int>(bounds.y + (badgeHeight - labelSize) * 0.5f);
        DrawText(badge.label.c_str(), textX, textY, labelSize, badge.textColor);

        statusBadgeX += badgeWidth + (8.0f * uiScale);
    }

    if (badgePopupIndex >= 0 && badgePopupIndex < static_cast<int>(allBadges.size())
            && !allBadges[badgePopupIndex].description.empty()) {
        drawBadgePopup(allBadges[badgePopupIndex], badgeRects[badgePopupIndex]);
    }

    BeginMode2D(camera);
    mandala->draw(colorPalette.getColors(), false);
    drawLastColorHintOverlay();
    inspector.drawValidationOverlay(*mandala, camera.zoom);
    drawAnalysisOverlay();
    drawDebugOverlay();
    EndMode2D();

    inspector.drawDebugInfoPanel(*mandala, uiScale);

    if (!readOnlyMode) {
        drawColorPalette();
    }

    backButton.draw();
    if (!readOnlyMode) {
        undoButton.draw();
        validateButton.draw();
    }

    if (inspector.isAnalysisMode()) {
        analysisCloseButton.draw();
        analysisClearButton.draw();
    } else {
        analysisButton.draw();
    }
}

void ColoringScreen::fitCameraToMandala() {
    const auto& regions = mandala->getRegions();
    if (regions.empty()) {
        return;
    }

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;

    bool hasVertex = false;
    for (const auto& region : regions) {
        const auto& vertices = region.getVertices();
        for (const auto& vertex : vertices) {
            minX = std::min(minX, vertex.x);
            minY = std::min(minY, vertex.y);
            maxX = std::max(maxX, vertex.x);
            maxY = std::max(maxY, vertex.y);
            hasVertex = true;
        }
    }

    if (!hasVertex) {
        return;
    }

    float contentWidth = std::max(maxX - minX, 1.0f);
    float contentHeight = std::max(maxY - minY, 1.0f);

    float availableWidth = std::max(1.0f, static_cast<float>(GetScreenWidth()) - (2.0f * CAMERA_FIT_MARGIN));
    float availableHeight = std::max(1.0f, static_cast<float>(GetScreenHeight()) - (2.0f * CAMERA_FIT_MARGIN));

    float fitZoomX = availableWidth / contentWidth;
    float fitZoomY = availableHeight / contentHeight;

    zoom = Clamp(std::min(fitZoomX, fitZoomY), MIN_ZOOM, MAX_ZOOM);
    camera.zoom = zoom;
    camera.target = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f};
}

void ColoringScreen::drawAnalysisOverlay() const {
    inspector.drawAnalysisOverlay(*mandala, colorPalette.getColors());
}

void ColoringScreen::drawDebugOverlay() const {
    inspector.drawDebugOverlay(*mandala);
}
