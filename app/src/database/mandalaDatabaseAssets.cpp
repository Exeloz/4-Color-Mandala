#include "mandalaDatabase.h"
#include "mandalaDatabaseAssetParsers.h"

#include <utility>

bool MandalaDatabase::loadManifest() {
    std::vector<ParsedManifestEntry> entries;
    bool parsedHardModeEnabled = false;
    if (!parseManifestFromAssets(entries, parsedHardModeEnabled)) {
        return false;
    }

    mandalaDescriptors.clear();
    mandalaListItems.clear();
    hardModeEnabled = parsedHardModeEnabled;

    for (const ParsedManifestEntry& entry : entries) {
        MandalaAssetDescriptor descriptor{};
        descriptor.id = entry.id;
        descriptor.name = entry.name;
        descriptor.regionsPath = entry.regionsPath;
        descriptor.adjacencyPath = entry.adjacencyPath;
        descriptor.hardAdjacencyPath = entry.hardAdjacencyPath;
        descriptor.minimumColors = entry.minimumColors;
        descriptor.minimumColorsHard = entry.minimumColorsHard;
        descriptor.availableFrom = entry.availableFrom;

        mandalaDescriptors.push_back(descriptor);
        mandalaListItems.push_back({descriptor.id,
                                    descriptor.name,
                                    !descriptor.hardAdjacencyPath.empty(),
                                    descriptor.availableFrom});
    }

    return true;
}

bool MandalaDatabase::loadMandalaFromAssets(const MandalaAssetDescriptor& descriptor, bool hardMode) {
    ParsedMandalaData parsedData;
    if (!loadMandalaDataFromAssets(descriptor.id,
                                   descriptor.regionsPath,
                                   descriptor.adjacencyPath,
                                   descriptor.hardAdjacencyPath,
                                   hardMode,
                                   parsedData)) {
        return false;
    }

    loadedMandalas.push_back({
        descriptor.id,
        hardMode,
        std::make_shared<Mandala>(descriptor.id,
                                  descriptor.name,
                                  parsedData.regions,
                                  parsedData.adjacencyGraph,
                                  parsedData.loadedRegionsPath,
                                  parsedData.loadedAdjacencyPath,
                                  hardMode,
                                  hardMode ? descriptor.minimumColorsHard : descriptor.minimumColors)
    });
    return true;
}
