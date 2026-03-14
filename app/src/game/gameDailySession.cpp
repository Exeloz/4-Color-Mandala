#include "game.h"
#include "dailyRuleset.h"

namespace {
std::vector<Color> buildPaletteForMode(const std::vector<Color>& fullPalette, bool hardMode) {
    if (hardMode) {
        return fullPalette;
    }

    const size_t normalSize = static_cast<size_t>(ColorPalette::LockedColorSlots + ColorPalette::NormalEditableColorCount);
    std::vector<Color> limited = fullPalette;
    if (limited.size() > normalSize) {
        limited.resize(normalSize);
    }
    return limited;
}

std::shared_ptr<Mandala> cloneMandala(const std::shared_ptr<Mandala>& src) {
    if (!src) {
        return nullptr;
    }
    return std::make_shared<Mandala>(*src);
}

void clearColorableRegions(const std::shared_ptr<Mandala>& mandala) {
    if (mandala == nullptr) {
        return;
    }

    for (const Region& regionView : mandala->getRegions()) {
        if (!regionView.isColorable()) {
            continue;
        }

        Region* mutableRegion = mandala->getRegionById(regionView.getId());
        if (mutableRegion != nullptr) {
            mutableRegion->setColor(-1);
        }
    }
}
}

std::vector<StatusBadge> Game::createDailyRuleBadges(const DailyRuleset& ruleset) const {
    std::vector<StatusBadge> ruleBadges;
    const std::string shortLabel = ruleset.getShortLabel();
    if (!shortLabel.empty()) {
        ruleBadges.push_back({shortLabel, ruleset.getDescription(),
            Color{147, 112, 219, 255},
            Color{80, 20, 160, 255},
            Color{255, 255, 255, 255},
            Color{0, 0, 0, 255}});
    }
    return ruleBadges;
}

bool Game::launchDailySession(uint64_t dateSeed,
                              const DailySelection& dailySelection,
                              GameScreenState returnState) {
    if (dailySelection.mandalaId < 0) {
        return false;
    }

    hardModeEnabled = dailySelection.hardMode;
    database->loadMandala(dailySelection.mandalaId, hardModeEnabled);
    selectedMandala = cloneMandala(database->getMandalaById(dailySelection.mandalaId, hardModeEnabled));
    if (selectedMandala == nullptr) {
        return false;
    }

    clearColorableRegions(selectedMandala);

    std::vector<Color> palette = buildPaletteForMode(appPaletteColors, hardModeEnabled);
    const DailyRuleset& ruleset = getDailyRulesetById(dailySelection.rulesetId);
    ruleset.applyToMandala(*selectedMandala, {dateSeed, dailySelection.mandalaId, hardModeEnabled});

    transientRandomSession = true;
    transientSessionReturnState = returnState;
    transientSessionKey = buildTransientSessionKey(
        dateSeed, dailySelection.mandalaId, hardModeEnabled, dailySelection.rulesetId);

    progressPersistence.applyToMandala(transientSessionKey, selectedMandala);

    bool openReadOnly = progressPersistence.isMandalaCompleted(transientSessionKey);
    if (openReadOnly) {
        std::vector<Color> frozenPalette;
        if (progressPersistence.tryGetMandalaFrozenPalette(transientSessionKey, frozenPalette)
            && !frozenPalette.empty()) {
            palette = frozenPalette;
        }
    }

    coloringScreen = std::make_shared<ColoringScreen>(selectedMandala,
                                                      palette,
                                                      openReadOnly,
                                                      createDailyRuleBadges(ruleset));
    suppressWinTransition = false;
    transitionToState(GameScreenState::COLORING);
    return true;
}
