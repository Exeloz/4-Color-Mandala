#include "dailyArchiveScreen.h"

#include "../game/dailyRuleset.h"
#include "../ui/colors.h"

#include "raymath.h"

#include <algorithm>

namespace {
float getUiScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / 960.0f;
    float heightScale = static_cast<float>(GetScreenHeight()) / 560.0f;
    return Clamp(std::min(widthScale, heightScale), 0.75f, 2.2f);
}
}

void DailyArchiveScreen::draw() {
    ClearBackground(Colors::Gainsboro);

    const float uiScale = getUiScale();

    const int titleSize = static_cast<int>(34.0f * uiScale);
    const char* title = "Daily Archive";
    const int titleWidth = MeasureText(title, titleSize);
    DrawText(title, (GetScreenWidth() - titleWidth) / 2, static_cast<int>(20.0f * uiScale), titleSize, Colors::Black);

    if (entries.empty()) {
        const char* emptyMessage = "No daily entries available yet.";
        const int msgSize = static_cast<int>(24.0f * uiScale);
        const int msgWidth = MeasureText(emptyMessage, msgSize);
        DrawText(emptyMessage,
                 (GetScreenWidth() - msgWidth) / 2,
                 GetScreenHeight() / 2 - msgSize,
                 msgSize,
                 Colors::Black);
        backButton.draw();
        return;
    }

    const int firstIndex = firstVisibleIndex();
    const int lastIndexExclusive = lastVisibleIndexExclusive();
    for (int i = firstIndex; i < lastIndexExclusive; ++i) {
        const Entry& entry = entries[static_cast<size_t>(i)];
        Button& button = entryButtons[static_cast<size_t>(i - firstIndex)];

        const DailyRuleset& ruleset = getDailyRulesetById(entry.selection.rulesetId);
        std::string mandalaName = "#" + std::to_string(entry.selection.mandalaId);
        if (database) {
            for (const auto& item : database->getMandalaListItems()) {
                if (item.id == entry.selection.mandalaId) {
                    mandalaName = item.name;
                    break;
                }
            }
        }
        button.setLabel(formatDate(entry.dateSeed) + " - " + mandalaName);

        if (entry.completed) {
            button.setColors(Color{62, 136, 74, 255}, Color{81, 162, 94, 255}, Colors::Black, Colors::White);
        } else {
            button.setColors(Color{61, 85, 130, 255}, Color{83, 110, 162, 255}, Colors::Black, Colors::White);
        }

        button.draw();

        {
            const Rectangle bounds = button.getBounds();
            const float badgePadX = 6.0f * uiScale;
            const float badgePadY = 4.0f * uiScale;
            const int badgeTextSize = static_cast<int>(12.0f * uiScale);
            float badgeX = bounds.x + bounds.width - 8.0f * uiScale;
            float badgeY = bounds.y + (bounds.height - badgeTextSize - badgePadY * 2.0f) * 0.5f;

            if (!ruleset.getShortLabel().empty()) {
                const std::string ruleLabel = ruleset.getShortLabel();
                const int ruleWidth = MeasureText(ruleLabel.c_str(), badgeTextSize);
                const float ruleBadgeW = ruleWidth + badgePadX * 2.0f;
                const float ruleBadgeH = badgeTextSize + badgePadY * 2.0f;
                badgeX -= ruleBadgeW;
                DrawRectangleRounded({badgeX, badgeY, ruleBadgeW, ruleBadgeH}, 0.4f, 4, Color{80, 120, 200, 255});
                DrawText(ruleLabel.c_str(),
                         static_cast<int>(badgeX + badgePadX),
                         static_cast<int>(badgeY + badgePadY),
                         badgeTextSize, Colors::White);
                badgeX -= 4.0f * uiScale;
            }

            if (entry.selection.hardMode) {
                const std::string hardLabel = "HARD";
                const int hardWidth = MeasureText(hardLabel.c_str(), badgeTextSize);
                const float hardBadgeW = hardWidth + badgePadX * 2.0f;
                const float hardBadgeH = badgeTextSize + badgePadY * 2.0f;
                badgeX -= hardBadgeW;
                DrawRectangleRounded({badgeX, badgeY, hardBadgeW, hardBadgeH}, 0.4f, 4, Color{200, 60, 60, 255});
                DrawText(hardLabel.c_str(),
                         static_cast<int>(badgeX + badgePadX),
                         static_cast<int>(badgeY + badgePadY),
                         badgeTextSize, Colors::White);
            }
        }

        if (i == selectedIndex) {
            const Rectangle bounds = button.getBounds();
            DrawRectangleRoundedLinesEx(bounds, 0.18f, 8, 3.0f, Color{255, 211, 77, 255});
        }
    }

    {
        const float navY = GetScreenHeight() - (58.0f * uiScale);
        const float navH = 46.0f * uiScale;
        const float navW = 124.0f * uiScale;
        const float margin = 18.0f * uiScale;
        const std::string pageLabel = std::to_string(pageIndex + 1) + " / " + std::to_string(std::max(1, pageCount()));
        const int pageSize = static_cast<int>(18.0f * uiScale);
        const int labelW = MeasureText(pageLabel.c_str(), pageSize);
        const float pillPadX = 12.0f * uiScale;
        const float pillPadY = 6.0f * uiScale;
        const float pillW = labelW + pillPadX * 2.0f;
        const float pillH = pageSize + pillPadY * 2.0f;
        const float pillX = GetScreenWidth() - margin - navW - 8.0f * uiScale - pillW;
        const float pillY = navY + (navH - pillH) * 0.5f;
        DrawRectangleRounded({pillX, pillY, pillW, pillH}, 0.5f, 8, Color{60, 60, 60, 200});
        DrawText(pageLabel.c_str(),
                 static_cast<int>(pillX + pillPadX),
                 static_cast<int>(pillY + pillPadY),
                 pageSize, Colors::White);
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(entries.size())) {
        const Entry& selectedEntry = entries[static_cast<size_t>(selectedIndex)];
        openButton.setLabel(selectedEntry.completed ? "VIEW COLORED" : "PLAY DAILY");
        if (selectedEntry.completed) {
            openButton.setColors(Color{62, 136, 74, 255}, Color{81, 162, 94, 255}, Colors::Black, Colors::White);
        } else {
            openButton.setColors(Color{57, 96, 168, 255}, Color{86, 124, 194, 255}, Colors::Black, Colors::White);
        }
        openButton.draw();
    }

    backButton.draw();
    prevPageButton.draw();
    nextPageButton.draw();
}

void DailyArchiveScreen::layoutControls() {
    const float uiScale = getUiScale();
    const float margin = 18.0f * uiScale;

    const float backWidth = 118.0f * uiScale;
    const float backHeight = 46.0f * uiScale;
    backButton.setPosition(margin, margin);
    backButton.setSize(backWidth, backHeight);

    const int count = entriesPerPage();
    const float rowWidth = GetScreenWidth() / 2.0f - 2.0f * margin;
    const float rowHeight = 52.0f * uiScale;
    const float top = 76.0f * uiScale;
    const float gap = 7.0f * uiScale;

    for (int i = 0; i < count; ++i) {
        entryButtons[static_cast<size_t>(i)].setPosition((GetScreenWidth() - rowWidth) / 2.0f,
                                                         top + static_cast<float>(i) * (rowHeight + gap));
        entryButtons[static_cast<size_t>(i)].setSize(rowWidth, rowHeight);
        entryButtons[static_cast<size_t>(i)].setTextScale(1.50f);
    }

    const float navY = GetScreenHeight() - (58.0f * uiScale);
    const float navW = 124.0f * uiScale;
    const float navH = 46.0f * uiScale;

    prevPageButton.setPosition(margin, navY);
    prevPageButton.setSize(navW, navH);

    nextPageButton.setPosition(GetScreenWidth() - margin - navW, navY);
    nextPageButton.setSize(navW, navH);

    backButton.setColors({72, 72, 78, 255}, {95, 95, 102, 255}, Colors::LightGray, Colors::White);

    prevPageButton.setColors({72, 72, 78, 255}, {95, 95, 102, 255}, Colors::LightGray, Colors::White);
    nextPageButton.setColors({72, 72, 78, 255}, {95, 95, 102, 255}, Colors::LightGray, Colors::White);
    prevPageButton.setEnabled(pageIndex > 0);
    nextPageButton.setEnabled(pageIndex < pageCount() - 1);

    const float openW = 170.0f * uiScale;
    openButton.setPosition((GetScreenWidth() - openW) * 0.5f, navY);
    openButton.setSize(openW, navH);
}
