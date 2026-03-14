#pragma once

#include "../mandala/mandala.h"

#include <string>
#include <vector>

struct ParsedManifestEntry {
    int id = -1;
    std::string name;
    std::string regionsPath;
    std::string adjacencyPath;
    std::string hardAdjacencyPath;
    int minimumColors = 0;
    int minimumColorsHard = 0;
    int availableFrom = 0;
};

struct ParsedMandalaData {
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph = AdjacencyGraph(0);
    std::string loadedRegionsPath;
    std::string loadedAdjacencyPath;
};

bool parseManifestFromAssets(std::vector<ParsedManifestEntry>& entries, bool& hardModeEnabled);

bool loadMandalaDataFromAssets(int mandalaId,
                               const std::string& regionsPath,
                               const std::string& adjacencyPath,
                               const std::string& hardAdjacencyPath,
                               bool hardMode,
                               ParsedMandalaData& outputData);
