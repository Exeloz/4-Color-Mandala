#include "progressPersistence.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace {
    constexpr const char* SAVE_FILE_NAME = "mandala_progress.dat";
    constexpr const char* SAVE_MAGIC = "MANDALA_PROGRESS_V1";

    std::string getSaveFilePath() {
#if defined(PLATFORM_ANDROID)
        const char* appDir = GetApplicationDirectory();
        if (appDir != nullptr && appDir[0] != '\0') {
            std::string resolvedPath = appDir;
            if (!resolvedPath.empty() && resolvedPath.back() != '/' && resolvedPath.back() != '\\') {
                resolvedPath += '/';
            }
            resolvedPath += SAVE_FILE_NAME;
            return resolvedPath;
        }
#endif
        return SAVE_FILE_NAME;
    }

    Color clampColorChannels(int red, int green, int blue, int alpha) {
        Color color{};
        color.r = static_cast<unsigned char>(std::max(0, std::min(255, red)));
        color.g = static_cast<unsigned char>(std::max(0, std::min(255, green)));
        color.b = static_cast<unsigned char>(std::max(0, std::min(255, blue)));
        color.a = static_cast<unsigned char>(std::max(0, std::min(255, alpha)));
        return color;
    }
}

ProgressPersistence::ProgressPersistence()
    : saveFilePath(getSaveFilePath()),
      savedPalette(),
      paletteAvailable(false),
      mandalaStates() {}

bool ProgressPersistence::load() {
    std::ifstream stream(saveFilePath);
    if (!stream.is_open()) {
        return false;
    }

    std::vector<Color> loadedPalette;
    std::unordered_map<int, PersistedMandalaState> loadedStates;
    bool loadedPaletteAvailable = false;

    std::string magic;
    if (!(stream >> magic) || magic != SAVE_MAGIC) {
        return false;
    }

    std::string paletteToken;
    int paletteCount = 0;
    if (!(stream >> paletteToken >> paletteCount) || paletteToken != "PALETTE" || paletteCount < 0) {
        return false;
    }

    loadedPalette.reserve(static_cast<size_t>(paletteCount));
    for (int i = 0; i < paletteCount; ++i) {
        int red = 0;
        int green = 0;
        int blue = 0;
        int alpha = 255;
        if (!(stream >> red >> green >> blue >> alpha)) {
            return false;
        }
        loadedPalette.push_back(clampColorChannels(red, green, blue, alpha));
    }
    loadedPaletteAvailable = (paletteCount > 0);

    std::string mandalasToken;
    int mandalaCount = 0;
    if (!(stream >> mandalasToken >> mandalaCount) || mandalasToken != "MANDALAS" || mandalaCount < 0) {
        return false;
    }

    for (int i = 0; i < mandalaCount; ++i) {
        std::string mandalaToken;
        int mandalaId = -1;
        int completed = 0;
        int regionCount = 0;

        if (!(stream >> mandalaToken >> mandalaId >> completed >> regionCount)
            || mandalaToken != "MANDALA"
            || regionCount < 0) {
            return false;
        }

        PersistedMandalaState state;
        state.completed = (completed != 0);

        for (int regionIndex = 0; regionIndex < regionCount; ++regionIndex) {
            std::string regionToken;
            int regionId = -1;
            int colorIndex = -1;

            if (!(stream >> regionToken >> regionId >> colorIndex) || regionToken != "REGION") {
                return false;
            }

            state.regionColors[regionId] = colorIndex;
        }

        loadedStates[mandalaId] = std::move(state);
    }

    savedPalette = std::move(loadedPalette);
    paletteAvailable = loadedPaletteAvailable;
    mandalaStates = std::move(loadedStates);
    return true;
}

bool ProgressPersistence::save() const {
    std::ofstream stream(saveFilePath, std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }

    stream << SAVE_MAGIC << "\n";
    stream << "PALETTE " << savedPalette.size() << "\n";
    for (const Color& color : savedPalette) {
        stream << static_cast<int>(color.r) << ' '
               << static_cast<int>(color.g) << ' '
               << static_cast<int>(color.b) << ' '
               << static_cast<int>(color.a) << "\n";
    }

    stream << "MANDALAS " << mandalaStates.size() << "\n";
    for (const auto& entry : mandalaStates) {
        const int mandalaId = entry.first;
        const PersistedMandalaState& state = entry.second;
        stream << "MANDALA "
               << mandalaId << ' '
               << (state.completed ? 1 : 0) << ' '
               << state.regionColors.size() << "\n";

        for (const auto& regionEntry : state.regionColors) {
            stream << "REGION " << regionEntry.first << ' ' << regionEntry.second << "\n";
        }
    }

    return stream.good();
}

void ProgressPersistence::setPalette(const std::vector<Color>& palette) {
    if (palette.empty()) {
        return;
    }

    savedPalette = palette;
    paletteAvailable = true;
}

const std::vector<Color>& ProgressPersistence::getPalette() const {
    return savedPalette;
}

bool ProgressPersistence::hasPalette() const {
    return paletteAvailable && !savedPalette.empty();
}

void ProgressPersistence::captureMandalaState(const Mandala& mandala) {
    PersistedMandalaState state;
    auto existingState = mandalaStates.find(mandala.getId());
    const bool wasCompleted = existingState != mandalaStates.end() && existingState->second.completed;

    for (const Region& region : mandala.getRegions()) {
        if (!region.isColorable()) {
            continue;
        }
        state.regionColors[region.getId()] = region.getColor();
    }

    state.completed = wasCompleted || mandala.isValidColoring();
    mandalaStates[mandala.getId()] = std::move(state);
}

void ProgressPersistence::captureAllMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas) {
    for (const std::shared_ptr<Mandala>& mandala : mandalas) {
        if (mandala == nullptr) {
            continue;
        }
        captureMandalaState(*mandala);
    }
}

void ProgressPersistence::applyToMandalas(const std::vector<std::shared_ptr<Mandala>>& mandalas) const {
    for (const std::shared_ptr<Mandala>& mandala : mandalas) {
        if (mandala == nullptr) {
            continue;
        }

        auto stateIterator = mandalaStates.find(mandala->getId());
        if (stateIterator == mandalaStates.end()) {
            continue;
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
}

bool ProgressPersistence::isMandalaCompleted(int mandalaId) const {
    auto iterator = mandalaStates.find(mandalaId);
    if (iterator == mandalaStates.end()) {
        return false;
    }

    return iterator->second.completed;
}

std::unordered_set<int> ProgressPersistence::getCompletedMandalaIds() const {
    std::unordered_set<int> completedIds;
    for (const auto& entry : mandalaStates) {
        if (entry.second.completed) {
            completedIds.insert(entry.first);
        }
    }
    return completedIds;
}