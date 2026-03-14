#include "mandalaDatabaseAssetParsers.h"

#include "mandalaDatabaseAssetJson.h"

#include <algorithm>
#include <limits>
#include <utility>

#include <raylib.h>

namespace {
constexpr float SCREEN_CENTER_X = 400.0f;
constexpr float SCREEN_CENTER_Y = 300.0f;

struct RawRegionData {
    int id = -1;
    std::vector<Vector2> vertices;
    bool hasColorable = false;
    bool colorable = true;
    bool hasDefaultColor = false;
    Color defaultColor = Color{0, 0, 0, 0};
};
}

bool loadMandalaDataFromAssets(int mandalaId,
                               const std::string& regionsPath,
                               const std::string& adjacencyPath,
                               const std::string& hardAdjacencyPath,
                               bool hardMode,
                               ParsedMandalaData& outputData) {
    std::string regionsText, loadedRegionsPath;
    if (!tryLoadTextFile(makeCandidateAssetPaths(regionsPath), regionsText, loadedRegionsPath)) {
        TraceLog(LOG_ERROR, "Unable to load regions file for mandala %d", mandalaId);
        return false;
    }

    JsonParser regionsParser(regionsText);
    JsonValue regionsRoot;
    try {
        regionsRoot = regionsParser.parseRoot();
    } catch (const std::exception& exception) {
        TraceLog(LOG_ERROR, "Invalid regions JSON (%s): %s", loadedRegionsPath.c_str(), exception.what());
        return false;
    }

    const JsonValue* regionsArray = &regionsRoot;
    if (regionsRoot.isObject()) {
        const JsonValue* embeddedRegions = regionsRoot.get("regions");
        if (embeddedRegions == nullptr || !embeddedRegions->isArray()) {
            TraceLog(LOG_ERROR, "Regions JSON object must contain array key 'regions': %s", loadedRegionsPath.c_str());
            return false;
        }
        regionsArray = embeddedRegions;
    }

    if (!regionsArray->isArray()) {
        TraceLog(LOG_ERROR, "Regions root must be an array: %s", loadedRegionsPath.c_str());
        return false;
    }

    std::vector<RawRegionData> rawRegions;
    rawRegions.reserve(regionsArray->arrayValue.size());

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (size_t index = 0; index < regionsArray->arrayValue.size(); ++index) {
        const JsonValue& regionValue = regionsArray->arrayValue[index];
        if (!regionValue.isObject()) {
            continue;
        }

        int regionId = static_cast<int>(index);
        const JsonValue* idValue = regionValue.get("id");
        if (idValue != nullptr && idValue->isNumber()) {
            regionId = static_cast<int>(idValue->numberValue);
        }

        const JsonValue* pointsValue = regionValue.get("points");
        if (pointsValue == nullptr) {
            continue;
        }

        std::vector<Vector2> vertices;
        if (!parseRegionVertices(*pointsValue, vertices)) {
            continue;
        }

        for (const Vector2& vertex : vertices) {
            minX = std::min(minX, vertex.x);
            minY = std::min(minY, vertex.y);
            maxX = std::max(maxX, vertex.x);
            maxY = std::max(maxY, vertex.y);
        }

        RawRegionData rawRegion;
        rawRegion.id = regionId;
        rawRegion.vertices = std::move(vertices);
        rawRegion.hasColorable = readBoolField(regionValue, "colorable", rawRegion.colorable);

        const JsonValue* defaultColorValue = regionValue.get("defaultColor");
        if (defaultColorValue != nullptr) {
            rawRegion.hasDefaultColor = parseColorField(*defaultColorValue, rawRegion.defaultColor);
            if (!rawRegion.hasDefaultColor) {
                TraceLog(LOG_WARNING, "Invalid defaultColor for region %d in %s", regionId, loadedRegionsPath.c_str());
            }
        }

        rawRegions.push_back(std::move(rawRegion));
    }

    if (rawRegions.empty()) {
        TraceLog(LOG_ERROR, "No valid regions in JSON: %s", loadedRegionsPath.c_str());
        return false;
    }

    const float sourceCenterX = (minX + maxX) * 0.5f;
    const float sourceCenterY = (minY + maxY) * 0.5f;
    const float offsetX = SCREEN_CENTER_X - sourceCenterX;
    const float offsetY = SCREEN_CENTER_Y - sourceCenterY;

    std::vector<Region> regions;
    regions.reserve(rawRegions.size());

    int maxRegionId = -1;
    for (const RawRegionData& entry : rawRegions) {
        maxRegionId = std::max(maxRegionId, entry.id);

        std::vector<Vector2> translatedVertices;
        translatedVertices.reserve(entry.vertices.size());
        for (const Vector2& vertex : entry.vertices) {
            translatedVertices.push_back(Vector2{vertex.x + offsetX, vertex.y + offsetY});
        }

        Region region(entry.id, translatedVertices);
        if (entry.hasColorable) {
            region.setColorable(entry.colorable);
        }
        if (entry.hasDefaultColor) {
            region.setDefaultColor(entry.defaultColor);
        }

        regions.emplace_back(std::move(region));
    }

    std::string chosenAdjacencyPath = adjacencyPath;
    if (hardMode) {
        if (hardAdjacencyPath.empty()) {
            TraceLog(LOG_WARNING,
                     "Mandala id %d requested in hard mode but no adjacency_hard path is defined",
                     mandalaId);
            return false;
        }
        chosenAdjacencyPath = hardAdjacencyPath;
    }

    std::string adjacencyText, loadedAdjacencyPath;
    if (!tryLoadTextFile(makeCandidateAssetPaths(chosenAdjacencyPath), adjacencyText, loadedAdjacencyPath)) {
        TraceLog(LOG_ERROR, "Unable to load adjacency file for mandala %d", mandalaId);
        return false;
    }

    JsonParser adjacencyParser(adjacencyText);
    JsonValue adjacencyRoot;
    try {
        adjacencyRoot = adjacencyParser.parseRoot();
    } catch (const std::exception& exception) {
        TraceLog(LOG_ERROR, "Invalid adjacency JSON (%s): %s", loadedAdjacencyPath.c_str(), exception.what());
        return false;
    }

    const JsonValue* pairsArray = &adjacencyRoot;
    if (adjacencyRoot.isObject()) {
        const JsonValue* embeddedPairs = adjacencyRoot.get("pairs");
        if (embeddedPairs == nullptr || !embeddedPairs->isArray()) {
            TraceLog(LOG_ERROR, "Adjacency JSON object must contain array key 'pairs': %s", loadedAdjacencyPath.c_str());
            return false;
        }
        pairsArray = embeddedPairs;
    }

    if (!pairsArray->isArray()) {
        TraceLog(LOG_ERROR, "Adjacency root must be an array: %s", loadedAdjacencyPath.c_str());
        return false;
    }

    AdjacencyGraph adjacencyGraph(std::max(static_cast<int>(regions.size()), maxRegionId + 1));
    for (const JsonValue& pairValue : pairsArray->arrayValue) {
        if (!pairValue.isArray() || pairValue.arrayValue.size() < 2) {
            continue;
        }

        const JsonValue& aValue = pairValue.arrayValue[0];
        const JsonValue& bValue = pairValue.arrayValue[1];
        if (!aValue.isNumber() || !bValue.isNumber()) {
            continue;
        }

        adjacencyGraph.addAdjacency(static_cast<int>(aValue.numberValue), static_cast<int>(bValue.numberValue));
    }

    outputData.regions = std::move(regions);
    outputData.adjacencyGraph = std::move(adjacencyGraph);
    outputData.loadedRegionsPath = std::move(loadedRegionsPath);
    outputData.loadedAdjacencyPath = std::move(loadedAdjacencyPath);
    return true;
}
