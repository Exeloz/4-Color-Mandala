#include "mandalaDatabaseAssetParsers.h"

#include "mandalaDatabaseAssetJson.h"

#include <utility>

#include <raylib.h>

bool parseManifestFromAssets(std::vector<ParsedManifestEntry>& entries, bool& hardModeEnabled) {
    std::string manifestText;
    std::string loadedPath;
    const std::vector<std::string> manifestCandidates = makeCandidateAssetPaths("mandalas_manifest.json");
    if (!tryLoadTextFile(manifestCandidates, manifestText, loadedPath)) {
        return false;
    }

    JsonParser parser(manifestText);
    JsonValue root;
    try {
        root = parser.parseRoot();
    } catch (const std::exception& exception) {
        TraceLog(LOG_ERROR, "Invalid manifest JSON (%s): %s", loadedPath.c_str(), exception.what());
        return false;
    }

    if (!root.isArray()) {
        TraceLog(LOG_ERROR, "Manifest root must be an array: %s", loadedPath.c_str());
        return false;
    }

    entries.clear();
    hardModeEnabled = false;

    for (const JsonValue& entry : root.arrayValue) {
        if (!entry.isObject()) {
            continue;
        }

        ParsedManifestEntry parsedEntry;
        if (!readIntField(entry, "id", parsedEntry.id)) {
            continue;
        }
        if (!readStringField(entry, "name", parsedEntry.name)) {
            continue;
        }
        if (!readStringField(entry, "regions", parsedEntry.regionsPath)) {
            continue;
        }
        if (!readStringField(entry, "adjacency", parsedEntry.adjacencyPath)) {
            continue;
        }

        readStringField(entry, "adjacency_hard", parsedEntry.hardAdjacencyPath);
        readIntField(entry, "min_colors", parsedEntry.minimumColors);
        if (!readIntField(entry, "min_colors_hard", parsedEntry.minimumColorsHard)) {
            parsedEntry.minimumColorsHard = parsedEntry.minimumColors;
        }
        readIntField(entry, "available_from", parsedEntry.availableFrom);

        if (!parsedEntry.hardAdjacencyPath.empty()) {
            hardModeEnabled = true;
        }

        entries.push_back(std::move(parsedEntry));
    }

    return true;
}
