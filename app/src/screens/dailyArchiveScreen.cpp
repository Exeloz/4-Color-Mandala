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

DailyArchiveScreen::DailyArchiveScreen(std::shared_ptr<MandalaDatabase> database,
                                       const ProgressPersistence& progressPersistence,
                                       uint64_t startDateSeed,
                                       uint64_t endDateSeed)
    : database(std::move(database)),
      entries(),
      entryButtons(),
    backButton(24, 24, 112, 46, "BACK"),
    prevPageButton(24, 24, 120, 46, "PREV"),
    nextPageButton(24, 24, 120, 46, "NEXT"),
    openButton(24, 24, 160, 46, "OPEN DAILY"),
      selectedIndex(-1),
      pageIndex(0),
      backRequested(false),
      launchRequested(false) {
    for (int i = 0; i < entriesPerPage(); ++i) {
        entryButtons.emplace_back(20.0f, 20.0f, 200.0f, 42.0f, "");
    }

    buildEntries(progressPersistence,
                 startDateSeed,
                 endDateSeed == 0ULL ? getCurrentLocalDateSeed() : endDateSeed);

    if (!entries.empty()) {
        selectedIndex = static_cast<int>(entries.size()) - 1;
        pageIndex = selectedIndex / entriesPerPage();
    }
}

void DailyArchiveScreen::update(float deltaTime) {
    (void)deltaTime;
    layoutControls();

    backButton.update();
    if (backButton.isClicked()) {
        backRequested = true;
    }

    prevPageButton.update();
    if (prevPageButton.isClicked() && pageIndex > 0) {
        pageIndex--;
    }

    nextPageButton.update();
    if (nextPageButton.isClicked() && pageIndex < pageCount() - 1) {
        pageIndex++;
    }

    const int firstIndex = firstVisibleIndex();
    const int lastIndexExclusive = lastVisibleIndexExclusive();
    for (int i = firstIndex; i < lastIndexExclusive; ++i) {
        Button& button = entryButtons[static_cast<size_t>(i - firstIndex)];
        button.update();
        if (button.isClicked()) {
            selectedIndex = i;
        }
    }

    openButton.update();
    if (openButton.isClicked() && selectedIndex >= 0 && selectedIndex < static_cast<int>(entries.size())) {
        launchRequested = true;
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
        const std::string mode = entry.selection.hardMode ? "Hard" : "Normal";
        const std::string status = entry.completed ? "Completed" : "Not completed";
        std::string mandalaName = "#" + std::to_string(entry.selection.mandalaId);
        if (database) {
            for (const auto& item : database->getMandalaListItems()) {
                if (item.id == entry.selection.mandalaId) { mandalaName = item.name; break; }
            }
        }
        button.setLabel(formatDate(entry.dateSeed)
                        + "  " + mandalaName
                        + "  " + mode
                        + "  " + ruleset.getName()
                        + "  " + status);

        if (entry.completed) {
            button.setColors(Color{62, 136, 74, 255}, Color{81, 162, 94, 255}, Colors::Black, Colors::White);
        } else {
            button.setColors(Color{61, 85, 130, 255}, Color{83, 110, 162, 255}, Colors::Black, Colors::White);
        }

        button.draw();

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
        // Place pill just to the left of the NEXT button
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

bool DailyArchiveScreen::consumeBackRequested() {
    if (!backRequested) {
        return false;
    }

    backRequested = false;
    return true;
}

bool DailyArchiveScreen::consumeLaunchRequested(DailyArchiveLaunchRequest& outRequest) {
    if (!launchRequested || selectedIndex < 0 || selectedIndex >= static_cast<int>(entries.size())) {
        return false;
    }

    launchRequested = false;
    const Entry& entry = entries[static_cast<size_t>(selectedIndex)];
    outRequest.dateSeed = entry.dateSeed;
    outRequest.selection = entry.selection;
    return true;
}

void DailyArchiveScreen::buildEntries(const ProgressPersistence& progressPersistence,
                                      uint64_t startDateSeed,
                                      uint64_t endDateSeed) {
    entries.clear();

    if (database == nullptr || startDateSeed > endDateSeed) {
        return;
    }

    uint64_t currentDate = startDateSeed;
    while (currentDate <= endDateSeed) {
        DailySelection selection = chooseDailyMandalaForDay(*database, currentDate);
        if (selection.mandalaId >= 0) {
            Entry entry;
            entry.dateSeed = currentDate;
            entry.selection = selection;
            entry.progressKey = buildTransientSessionKey(currentDate,
                                                         selection.mandalaId,
                                                         selection.hardMode,
                                                         selection.rulesetId);
            entry.completed = progressPersistence.isMandalaCompleted(entry.progressKey);
            entries.push_back(std::move(entry));
        }

        const uint64_t next = nextDateSeed(currentDate);
        if (next <= currentDate) {
            break;
        }
        currentDate = next;
    }
}

void DailyArchiveScreen::layoutControls() {
    const float uiScale = getUiScale();
    const float margin = 18.0f * uiScale;

    const float backWidth = 118.0f * uiScale;
    const float backHeight = 46.0f * uiScale;
    backButton.setPosition(margin, margin);
    backButton.setSize(backWidth, backHeight);

    const int count = entriesPerPage();
    const float rowWidth = GetScreenWidth()/2.0f - 2.0f * margin;
    const float rowHeight = 52.0f * uiScale;
    const float top = 76.0f * uiScale;
    const float gap = 7.0f * uiScale;

    for (int i = 0; i < count; ++i) {
        entryButtons[static_cast<size_t>(i)].setPosition((GetScreenWidth()-rowWidth)/2.0f, top + static_cast<float>(i) * (rowHeight + gap));
        entryButtons[static_cast<size_t>(i)].setSize(rowWidth, rowHeight);
        entryButtons[static_cast<size_t>(i)].setTextScale(2.0f);
    }

    const float navY = GetScreenHeight() - (58.0f * uiScale);
    const float navW = 124.0f * uiScale;
    const float navH = 46.0f * uiScale;

    prevPageButton.setPosition(margin, navY);
    prevPageButton.setSize(navW, navH);

    nextPageButton.setPosition(GetScreenWidth() - margin - navW, navY);
    nextPageButton.setSize(navW, navH);

    // BACK: neutral dark gray
    backButton.setColors({72, 72, 78, 255}, {95, 95, 102, 255}, Colors::LightGray, Colors::White);

    // PREV / NEXT: dark gray, disabled at page boundaries
    prevPageButton.setColors({72, 72, 78, 255}, {95, 95, 102, 255}, Colors::LightGray, Colors::White);
    nextPageButton.setColors({72, 72, 78, 255}, {95, 95, 102, 255}, Colors::LightGray, Colors::White);
    prevPageButton.setEnabled(pageIndex > 0);
    nextPageButton.setEnabled(pageIndex < pageCount() - 1);

    const float openW = 170.0f * uiScale;
    openButton.setPosition((GetScreenWidth() - openW) * 0.5f, navY);
    openButton.setSize(openW, navH);
}

int DailyArchiveScreen::entriesPerPage() const {
    return 6;
}

int DailyArchiveScreen::pageCount() const {
    if (entries.empty()) {
        return 1;
    }
    const int count = static_cast<int>(entries.size());
    const int perPage = entriesPerPage();
    return (count + perPage - 1) / perPage;
}

int DailyArchiveScreen::firstVisibleIndex() const {
    return pageIndex * entriesPerPage();
}

int DailyArchiveScreen::lastVisibleIndexExclusive() const {
    const int last = firstVisibleIndex() + entriesPerPage();
    return std::min(static_cast<int>(entries.size()), last);
}

std::string DailyArchiveScreen::formatDate(uint64_t dateSeed) const {
    const int year = static_cast<int>(dateSeed / 10000ULL);
    const int month = static_cast<int>((dateSeed / 100ULL) % 100ULL);
    const int day = static_cast<int>(dateSeed % 100ULL);

    std::string monthStr = month < 10 ? ("0" + std::to_string(month)) : std::to_string(month);
    std::string dayStr = day < 10 ? ("0" + std::to_string(day)) : std::to_string(day);
    return std::to_string(year) + "-" + monthStr + "-" + dayStr;
}

bool DailyArchiveScreen::isLeapYear(int year) const {
    if (year % 400 == 0) {
        return true;
    }
    if (year % 100 == 0) {
        return false;
    }
    return (year % 4 == 0);
}

int DailyArchiveScreen::daysInMonth(int year, int month) const {
    static const int dayCounts[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return dayCounts[month - 1];
}

uint64_t DailyArchiveScreen::nextDateSeed(uint64_t currentDate) const {
    int year = static_cast<int>(currentDate / 10000ULL);
    int month = static_cast<int>((currentDate / 100ULL) % 100ULL);
    int day = static_cast<int>(currentDate % 100ULL);

    day++;
    if (day > daysInMonth(year, month)) {
        day = 1;
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }

    return static_cast<uint64_t>(year) * 10000ULL
           + static_cast<uint64_t>(month) * 100ULL
           + static_cast<uint64_t>(day);
}
