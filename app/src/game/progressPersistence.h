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
    void setPalette(const std::vector<Color>& palette, bool hardMode);
    const std::vector<Color>& getPalette() const;
    const std::vector<Color>& getPalette(bool hardMode) const;
    bool hasPalette() const;
    bool hasPalette(bool hardMode) const;

    void captureMandalaState(const Mandala& mandala, const std::vector<Color>& activePalette);
    void captureMandalaState(const std::string& mandalaKey,
                             const Mandala& mandala,
                             const std::vector<Color>& activePalette);

    void captureAllMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas,
                            const std::vector<Color>& activePalette);

    void applyToMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas) const;
    void applyToMandala(const std::string& mandalaKey, const std::shared_ptr<Mandala>& mandala) const;

    void clearMandalaState(int mandalaId);
    void clearMandalaState(const std::string& mandalaKey);

    bool isMandalaCompleted(int mandalaId) const;
    bool isMandalaCompleted(const std::string& mandalaKey) const;

    bool tryGetMandalaFrozenPalette(int mandalaId, std::vector<Color>& outPalette) const;
    bool tryGetMandalaFrozenPalette(const std::string& mandalaKey, std::vector<Color>& outPalette) const;

    std::unordered_set<int> getCompletedMandalaIds() const;
    std::unordered_set<int> getCompletedMandalaIds(bool hardMode) const;

    static std::string makeMandalaKey(int mandalaId, bool hardMode);

private:
    std::string saveFilePath;
    std::vector<Color> savedPalette;
    std::vector<Color> savedPaletteHard;
    bool paletteAvailable;
    bool paletteHardAvailable;
    std::unordered_map<std::string, PersistedMandalaState> mandalaStates;
};