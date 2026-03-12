#pragma once

#include "../database/mandalaDatabase.h"
#include "../game/dailySelection.h"
#include "../game/gameState.h"
#include "../game/progressPersistence.h"
#include "../ui/button.h"

#include <memory>
#include <string>
#include <vector>

struct DailyArchiveLaunchRequest {
    uint64_t dateSeed = 0;
    DailySelection selection;
};

class DailyArchiveScreen : public GameState {
public:
    DailyArchiveScreen(std::shared_ptr<MandalaDatabase> database,
                       const ProgressPersistence& progressPersistence,
                       uint64_t startDateSeed = 20260301ULL,
                       uint64_t endDateSeed = 0ULL);

    void update(float deltaTime) override;
    void draw() override;

    bool consumeBackRequested();
    bool consumeLaunchRequested(DailyArchiveLaunchRequest& outRequest);

private:
    struct Entry {
        uint64_t dateSeed = 0;
        DailySelection selection;
        std::string progressKey;
        bool completed = false;
    };

    std::shared_ptr<MandalaDatabase> database;
    std::vector<Entry> entries;
    std::vector<Button> entryButtons;
    Button backButton;
    Button prevPageButton;
    Button nextPageButton;
    Button openButton;

    int selectedIndex;
    int pageIndex;
    bool backRequested;
    bool launchRequested;

    void buildEntries(const ProgressPersistence& progressPersistence,
                      uint64_t startDateSeed,
                      uint64_t endDateSeed);
    void layoutControls();
    int entriesPerPage() const;
    int pageCount() const;
    int firstVisibleIndex() const;
    int lastVisibleIndexExclusive() const;
    std::string formatDate(uint64_t dateSeed) const;
    bool isLeapYear(int year) const;
    int daysInMonth(int year, int month) const;
    uint64_t nextDateSeed(uint64_t currentDate) const;
};
