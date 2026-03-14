#include "dailyArchiveScreen.h"

#include <algorithm>

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
