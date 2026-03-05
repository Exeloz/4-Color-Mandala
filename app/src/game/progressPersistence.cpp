#include "progressPersistence.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
    constexpr const char* SAVE_FILE_NAME = "mandala_progress.dat";
    constexpr const char* SAVE_MAGIC = "MANDALA_PROGRESS_V1";
    constexpr const char* PALETTE_SECTION = "PALETTE";
    constexpr const char* PALETTE_HARD_SECTION = "PALETTE_HARD";
    constexpr const char* MANDALAS_SECTION = "MANDALAS";

    Color clampColorChannels(int red, int green, int blue, int alpha) {
        Color color{};
        color.r = static_cast<unsigned char>(std::max(0, std::min(255, red)));
        color.g = static_cast<unsigned char>(std::max(0, std::min(255, green)));
        color.b = static_cast<unsigned char>(std::max(0, std::min(255, blue)));
        color.a = static_cast<unsigned char>(std::max(0, std::min(255, alpha)));
        return color;
    }

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

    bool readPalette(std::istringstream& stream, int count, std::vector<Color>& outputPalette) {
        if (count < 0) {
            return false;
        }

        outputPalette.clear();
        outputPalette.reserve(static_cast<size_t>(count));

        for (int i = 0; i < count; ++i) {
            int red = 0;
            int green = 0;
            int blue = 0;
            int alpha = 255;
            if (!(stream >> red >> green >> blue >> alpha)) {
                return false;
            }
            outputPalette.push_back(clampColorChannels(red, green, blue, alpha));
        }

        return true;
    }
}

ProgressPersistence::ProgressPersistence()
    : saveFilePath(SAVE_FILE_NAME),
      savedPalette(),
      paletteAvailable(false),
      mandalaStates() {}

std::string ProgressPersistence::makeMandalaKey(int mandalaId, bool hardMode) {
    return hardMode ? (std::to_string(mandalaId) + "H") : std::to_string(mandalaId);
}

bool ProgressPersistence::load() {
    char* loadedText = LoadFileText(saveFilePath.c_str());
    if (loadedText == nullptr) {
        return false;
    }

    std::istringstream stream(loadedText);
    UnloadFileText(loadedText);

    std::vector<Color> loadedPalette;
    std::unordered_map<std::string, PersistedMandalaState> loadedStates;
    bool loadedPaletteAvailable = false;

    std::string magic;
    if (!(stream >> magic) || magic != SAVE_MAGIC) {
        return false;
    }

    std::string paletteToken;
    int paletteCount = 0;
    if (!(stream >> paletteToken >> paletteCount) || paletteToken != PALETTE_SECTION) {
        return false;
    }

    if (!readPalette(stream, paletteCount, loadedPalette)) {
        return false;
    }
    loadedPaletteAvailable = (paletteCount > 0);

    std::string sectionToken;
    int sectionCount = 0;
    if (!(stream >> sectionToken >> sectionCount)) {
        return false;
    }

    if (sectionToken == PALETTE_HARD_SECTION) {
        std::vector<Color> ignoredHardPalette;
        if (!readPalette(stream, sectionCount, ignoredHardPalette)) {
            return false;
        }
        (void)ignoredHardPalette;

        if (!(stream >> sectionToken >> sectionCount)) {
            return false;
        }
    }

    if (sectionToken != MANDALAS_SECTION || sectionCount < 0) {
        return false;
    }

    const int mandalaCount = sectionCount;

    for (int i = 0; i < mandalaCount; ++i) {
        std::string mandalaHeaderLine;
        if (!std::getline(stream >> std::ws, mandalaHeaderLine)) {
            return false;
        }

        std::istringstream mandalaHeaderStream(mandalaHeaderLine);
        std::string mandalaToken;
        std::string mandalaKey;
        int completed = 0;
        int regionCount = 0;
        int frozenPaletteCount = 0;

        if (!(mandalaHeaderStream >> mandalaToken >> mandalaKey >> completed >> regionCount)
            || mandalaToken != "MANDALA"
            || mandalaKey.empty()
            || regionCount < 0) {
            return false;
        }

        if (mandalaHeaderStream >> frozenPaletteCount) {
            if (frozenPaletteCount < 0) {
                return false;
            }
        } else {
            frozenPaletteCount = 0;
        }

        PersistedMandalaState state;
        state.completed = (completed != 0);

        if (frozenPaletteCount > 0) {
            state.frozenPalette.reserve(static_cast<size_t>(frozenPaletteCount));
            for (int paletteIndex = 0; paletteIndex < frozenPaletteCount; ++paletteIndex) {
                int red = 0;
                int green = 0;
                int blue = 0;
                int alpha = 255;
                if (!(stream >> red >> green >> blue >> alpha)) {
                    return false;
                }
                state.frozenPalette.push_back(clampColorChannels(red, green, blue, alpha));
            }
        }

        for (int regionIndex = 0; regionIndex < regionCount; ++regionIndex) {
            std::string regionToken;
            int regionId = -1;
            int colorIndex = -1;

            if (!(stream >> regionToken >> regionId >> colorIndex) || regionToken != "REGION") {
                return false;
            }

            state.regionColors[regionId] = colorIndex;
        }

        loadedStates[mandalaKey] = std::move(state);
    }

    savedPalette = std::move(loadedPalette);
    paletteAvailable = loadedPaletteAvailable;
    mandalaStates = std::move(loadedStates);
    return true;
}

bool ProgressPersistence::save() const {
    std::ostringstream stream;

    stream << SAVE_MAGIC << "\n";
    stream << PALETTE_SECTION << ' ' << savedPalette.size() << "\n";
    for (const Color& color : savedPalette) {
        stream << static_cast<int>(color.r) << ' '
               << static_cast<int>(color.g) << ' '
               << static_cast<int>(color.b) << ' '
               << static_cast<int>(color.a) << "\n";
    }

    stream << MANDALAS_SECTION << ' ' << mandalaStates.size() << "\n";
    for (const auto& entry : mandalaStates) {
        const std::string& mandalaKey = entry.first;
        const PersistedMandalaState& state = entry.second;
        stream << "MANDALA "
               << mandalaKey << ' '
               << (state.completed ? 1 : 0) << ' '
               << state.regionColors.size() << ' '
               << state.frozenPalette.size() << "\n";

        for (const Color& color : state.frozenPalette) {
            stream << static_cast<int>(color.r) << ' '
                   << static_cast<int>(color.g) << ' '
                   << static_cast<int>(color.b) << ' '
                   << static_cast<int>(color.a) << "\n";
        }

        for (const auto& regionEntry : state.regionColors) {
            stream << "REGION " << regionEntry.first << ' ' << regionEntry.second << "\n";
        }
    }

    const std::string serializedData = stream.str();
    return SaveFileText(saveFilePath.c_str(), serializedData.c_str());
}

void ProgressPersistence::setPalette(const std::vector<Color>& palette) {
    setPalette(palette, false);
}

void ProgressPersistence::setPalette(const std::vector<Color>& palette, bool hardMode) {
    (void)hardMode;
    if (palette.empty()) {
        return;
    }

    savedPalette = palette;
    paletteAvailable = true;
}

const std::vector<Color>& ProgressPersistence::getPalette() const {
    return getPalette(false);
}

const std::vector<Color>& ProgressPersistence::getPalette(bool hardMode) const {
    (void)hardMode;
    return savedPalette;
}

bool ProgressPersistence::hasPalette() const {
    return hasPalette(false);
}

bool ProgressPersistence::hasPalette(bool hardMode) const {
    (void)hardMode;
    return paletteAvailable && !savedPalette.empty();
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
        state.regionColors[region.getId()] = region.getColor();
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
