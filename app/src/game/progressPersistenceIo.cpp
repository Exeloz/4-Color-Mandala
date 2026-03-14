#include "progressPersistence.h"

#include <algorithm>
#include <sstream>

namespace {
constexpr const char* SAVE_MAGIC_V1 = "MANDALA_PROGRESS_V1";
constexpr const char* SAVE_MAGIC_V2 = "MANDALA_PROGRESS_V2";
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
    if (!(stream >> magic) || (magic != SAVE_MAGIC_V1 && magic != SAVE_MAGIC_V2)) {
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

        if (!(stream >> sectionToken >> sectionCount)) {
            return false;
        }
    }

    if (sectionToken != MANDALAS_SECTION || sectionCount < 0) {
        return false;
    }

    for (int i = 0; i < sectionCount; ++i) {
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

    stream << SAVE_MAGIC_V2 << "\n";
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
