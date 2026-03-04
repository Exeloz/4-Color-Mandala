#pragma once

#include "../mandala/mandala.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PersistedMandalaState {
    std::unordered_map<int, int> regionColors;
    bool completed = false;
    std::vector<Color> frozenPalette;
};

class ProgressPersistence {
public:
    ProgressPersistence();

    bool load();
    bool save() const;

    void setPalette(const std::vector<Color>& palette);
    const std::vector<Color>& getPalette() const;
    bool hasPalette() const;

    void captureMandalaState(const Mandala& mandala, const std::vector<Color>& activePalette);
    void captureAllMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas,
                            const std::vector<Color>& activePalette);
    void applyToMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas) const;
    void clearMandalaState(int mandalaId);

    bool isMandalaCompleted(int mandalaId) const;
    bool tryGetMandalaFrozenPalette(int mandalaId, std::vector<Color>& outPalette) const;
    std::unordered_set<int> getCompletedMandalaIds() const;

private:
    std::string saveFilePath;
    std::vector<Color> savedPalette;
    bool paletteAvailable;
    std::unordered_map<int, PersistedMandalaState> mandalaStates;
};