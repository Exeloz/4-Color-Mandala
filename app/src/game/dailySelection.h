#pragma once

#include "../database/mandalaDatabase.h"
#include <cstdint>
#include <string>

struct DailySelection {
    int mandalaId = -1;
    bool hardMode = false;
    int rulesetId = 0;
};

uint64_t getCurrentLocalDateSeed();
DailySelection chooseDailyMandalaForDay(const MandalaDatabase& database, uint64_t targetDate);
std::string buildTransientSessionKey(uint64_t dateYYYYMMDD, int mandalaId, bool hardMode, int rulesetId);
