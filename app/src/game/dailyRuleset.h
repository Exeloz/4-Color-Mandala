#pragma once
#include "../mandala/mandala.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct DailyRuleContext {
    uint64_t daySeed = 0;   // YYYYMMDD
    int mandalaId = -1;
    bool hardMode = false;
};

// Base class for optional rule modifiers applied on top of the base daily session.
// Each ruleset has a stable numeric id used in score hashing and in save keys.
// To add a new ruleset: subclass DailyRuleset, give it a unique id, register it in
// dailyRuleset.cpp's getAllDailyRulesets().
class DailyRuleset {
public:
    virtual ~DailyRuleset() = default;

    // Stable identifier — never reuse a retired id.
    virtual int getId() const = 0;

    // Human-readable label shown in the UI (empty string = no label shown).
    virtual std::string getName() const = 0;

    // Short uppercase label shown as a badge (e.g. "HINTS").  Empty = no badge.
    virtual std::string getShortLabel() const { return ""; }

    // One or two lines of description shown in the badge tooltip popup.
    // Use \n to split lines.
    virtual std::string getDescription() const { return ""; }

    // A unique 64-bit mixing constant baked into the hash for this ruleset.
    // Must differ from every other ruleset's constant.
    virtual uint64_t hashContribution() const = 0;

    // Applies this ruleset to a freshly loaded mandala before player interaction.
    virtual void applyToMandala(Mandala& mandala, const DailyRuleContext& context) const = 0;
};

// ----- Concrete rulesets ------------------------------------------------

// Freeze puzzle: use a stored valid solution and lock a subset of regions to those colors.
// This is the only active Daily ruleset for now.
class SolutionFreezeDailyRuleset : public DailyRuleset {
public:
    int      getId()              const override { return 0; }
    std::string getName()         const override { return "Freeze 50"; }
    std::string getShortLabel()   const override { return "HINTS"; }
    std::string getDescription()  const override {
        return "Some regions are pre-colored\nwith correct solution colors.";
    }
    uint64_t hashContribution()   const override { return 0x517cc1b727220a95ULL; }
    void applyToMandala(Mandala& mandala, const DailyRuleContext& context) const override;

    int defaultFrozenRegionCount() const { return 50; }
};

// Last-color hint puzzle: shows a dynamic count in regions for how many adjacent
// regions still need to be the solution's highest-index color.
class LastColorHintDailyRuleset : public DailyRuleset {
public:
    int getId() const override { return 1; }
    std::string getName() const override { return "Last Color Hints"; }
    std::string getShortLabel() const override { return "LAST"; }
    std::string getDescription() const override {
        return "Numbers show nearby regions\nthat still need the last color.";
    }
    uint64_t hashContribution() const override { return 0x1c9b4dbe6a4f9283ULL; }
    void applyToMandala(Mandala& mandala, const DailyRuleContext& context) const override;
};

// -----------------------------------------------------------------------

// Returns every registered ruleset in a stable order.
// This is the single source of truth used by the daily selector.
const std::vector<std::shared_ptr<DailyRuleset>>& getAllDailyRulesets();

// Looks up a ruleset by id.  Returns the DefaultDailyRuleset if id is unknown.
const DailyRuleset& getDailyRulesetById(int id);
