#include "dailySelection.h"
#include "dailyRuleset.h"

#include <ctime>
#include <vector>

namespace {
uint64_t mix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

uint64_t scoreCandidate(uint64_t daySeed, int mandalaId, bool hardMode, const DailyRuleset& ruleset) {
    uint64_t key = daySeed;
    key ^= static_cast<uint64_t>(mandalaId) * 0x9e3779b185ebca87ULL;
    key ^= hardMode ? 0xa0761d6478bd642fULL : 0xe7037ed1a0b428dbULL;
    key ^= ruleset.hashContribution();
    return mix64(key);
}
}

uint64_t getCurrentLocalDateSeed() {
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    const uint64_t year = static_cast<uint64_t>(local->tm_year + 1900);
    const uint64_t month = static_cast<uint64_t>(local->tm_mon + 1);
    const uint64_t day = static_cast<uint64_t>(local->tm_mday);
    return year * 10000ULL + month * 100ULL + day;
}

DailySelection chooseDailyMandalaForDay(const MandalaDatabase& database, uint64_t targetDate) {
    const auto& items = database.getMandalaListItems();
    const auto& rulesets = getAllDailyRulesets();

    DailySelection best{};
    uint64_t bestScore = 0;
    bool hasBest = false;

    for (const auto& item : items) {
        if (item.id == 0) {
            continue;
        }

        if (item.availableFrom > 0 && static_cast<uint64_t>(item.availableFrom) > targetDate) {
            continue;
        }

        for (const auto& ruleset : rulesets) {
            const uint64_t normalScore = scoreCandidate(targetDate, item.id, false, *ruleset);
            if (!hasBest || normalScore > bestScore) {
                best = {item.id, false, ruleset->getId()};
                bestScore = normalScore;
                hasBest = true;
            }

            if (item.hasHardMode) {
                const uint64_t hardScore = scoreCandidate(targetDate, item.id, true, *ruleset);
                if (!hasBest || hardScore > bestScore) {
                    best = {item.id, true, ruleset->getId()};
                    bestScore = hardScore;
                    hasBest = true;
                }
            }
        }
    }

    return hasBest ? best : DailySelection{};
}

std::string buildTransientSessionKey(uint64_t dateYYYYMMDD, int mandalaId, bool hardMode, int rulesetId) {
    const int year = static_cast<int>(dateYYYYMMDD / 10000);
    const int month = static_cast<int>((dateYYYYMMDD / 100) % 100);
    const int day = static_cast<int>(dateYYYYMMDD % 100);
    return "R_" + std::to_string(year)
           + "_" + std::to_string(month)
           + "_" + std::to_string(day)
           + "_" + std::to_string(mandalaId)
           + (hardMode ? "_H" : "_N")
           + "_" + std::to_string(rulesetId);
}
