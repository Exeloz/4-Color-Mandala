#include "progressPersistence.h"

#include <cctype>

namespace {
bool isHardKey(const std::string& key) {
    return !key.empty() && key.back() == 'H';
}

bool tryParseBaseMandalaId(const std::string& key, int& outId) {
    outId = -1;
    if (key.empty()) {
        return false;
    }

    std::string numericPart = key;
    if (isHardKey(numericPart)) {
        numericPart.pop_back();
    }
    if (numericPart.empty()) {
        return false;
    }

    for (char c : numericPart) {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
            return false;
        }
    }

    outId = std::stoi(numericPart);
    return true;
}
}

void ProgressPersistence::captureMandalaState(const Mandala& mandala, const std::vector<Color>& activePalette) {
    captureMandalaState(makeMandalaKey(mandala.getId(), false), mandala, activePalette);
}

void ProgressPersistence::captureMandalaState(const std::string& mandalaKey,
                                             const Mandala& mandala,
                                             const std::vector<Color>& activePalette) {
    PersistedMandalaState state;
    auto existingState = mandalaStates.find(mandalaKey);
    const bool wasCompleted = existingState != mandalaStates.end() && existingState->second.completed;
    if (existingState != mandalaStates.end()) {
        state.frozenPalette = existingState->second.frozenPalette;
    }

    for (const Region& region : mandala.getRegions()) {
        if (!region.isColorable()) {
            continue;
        }

        if (region.getColor() >= 0) {
            state.regionColors[region.getId()] = region.getColor();
        }
    }

    state.completed = wasCompleted || mandala.isValidColoring();
    if (state.completed && state.frozenPalette.empty() && !activePalette.empty()) {
        state.frozenPalette = activePalette;
    }

    mandalaStates[mandalaKey] = std::move(state);
}

void ProgressPersistence::captureAllMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas,
                                             const std::vector<Color>& activePalette) {
    for (const std::shared_ptr<Mandala>& mandala : mandalas) {
        if (mandala == nullptr) {
            continue;
        }
        captureMandalaState(makeMandalaKey(mandala->getId(), false), *mandala, activePalette);
    }
}

void ProgressPersistence::applyToMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas) const {
    for (const std::shared_ptr<Mandala>& mandala : mandalas) {
        if (mandala == nullptr) {
            continue;
        }
        applyToMandala(makeMandalaKey(mandala->getId(), false), mandala);
    }
}

void ProgressPersistence::applyToMandala(const std::string& mandalaKey, const std::shared_ptr<Mandala>& mandala) const {
    if (mandala == nullptr) {
        return;
    }

    auto stateIterator = mandalaStates.find(mandalaKey);
    if (stateIterator == mandalaStates.end()) {
        return;
    }

    for (const Region& regionView : mandala->getRegions()) {
        if (!regionView.isColorable()) {
            continue;
        }

        Region* region = mandala->getRegionById(regionView.getId());
        if (region != nullptr) {
            region->setColor(-1);
        }
    }

    const PersistedMandalaState& state = stateIterator->second;
    for (const auto& regionEntry : state.regionColors) {
        Region* region = mandala->getRegionById(regionEntry.first);
        if (region == nullptr || !region->isColorable()) {
            continue;
        }

        region->setColor(regionEntry.second);
    }
}

void ProgressPersistence::clearMandalaState(int mandalaId) {
    clearMandalaState(makeMandalaKey(mandalaId, false));
}

void ProgressPersistence::clearMandalaState(const std::string& mandalaKey) {
    mandalaStates.erase(mandalaKey);
}

bool ProgressPersistence::isMandalaCompleted(int mandalaId) const {
    return isMandalaCompleted(makeMandalaKey(mandalaId, false));
}

bool ProgressPersistence::isMandalaCompleted(const std::string& mandalaKey) const {
    auto iterator = mandalaStates.find(mandalaKey);
    if (iterator == mandalaStates.end()) {
        return false;
    }

    return iterator->second.completed;
}

bool ProgressPersistence::tryGetMandalaFrozenPalette(int mandalaId, std::vector<Color>& outPalette) const {
    return tryGetMandalaFrozenPalette(makeMandalaKey(mandalaId, false), outPalette);
}

bool ProgressPersistence::tryGetMandalaFrozenPalette(const std::string& mandalaKey, std::vector<Color>& outPalette) const {
    outPalette.clear();

    auto iterator = mandalaStates.find(mandalaKey);
    if (iterator == mandalaStates.end() || iterator->second.frozenPalette.empty()) {
        return false;
    }

    outPalette = iterator->second.frozenPalette;
    return true;
}

std::unordered_set<int> ProgressPersistence::getCompletedMandalaIds() const {
    return getCompletedMandalaIds(false);
}

std::unordered_set<int> ProgressPersistence::getCompletedMandalaIds(bool hardMode) const {
    std::unordered_set<int> completedIds;
    for (const auto& entry : mandalaStates) {
        if (!entry.second.completed) {
            continue;
        }

        const std::string& key = entry.first;
        if (isHardKey(key) != hardMode) {
            continue;
        }

        int baseMandalaId = -1;
        if (tryParseBaseMandalaId(key, baseMandalaId)) {
            completedIds.insert(baseMandalaId);
        }
    }
    return completedIds;
}
