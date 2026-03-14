#include "mandalaDatabaseAssetJson.h"

#include <raylib.h>

bool tryLoadTextFile(const std::vector<std::string>& candidatePaths,
                     std::string& outputText,
                     std::string& loadedPath) {
    for (const std::string& path : candidatePaths) {
        char* rawData = LoadFileText(path.c_str());
        if (rawData == nullptr) {
            continue;
        }

        outputText = rawData;
        loadedPath = path;
        UnloadFileText(rawData);
        return true;
    }

    return false;
}

std::vector<std::string> makeCandidateAssetPaths(const std::string& relativePath) {
    std::vector<std::string> candidates;
    candidates.emplace_back(relativePath);
    candidates.emplace_back("resources/assets/" + relativePath);
    candidates.emplace_back("assets/" + relativePath);
    return candidates;
}

bool readStringField(const JsonValue& object, const std::string& key, std::string& output) {
    const JsonValue* value = object.get(key);
    if (value == nullptr || !value->isString()) {
        return false;
    }

    output = value->stringValue;
    return true;
}

bool readIntField(const JsonValue& object, const std::string& key, int& output) {
    const JsonValue* value = object.get(key);
    if (value == nullptr || !value->isNumber()) {
        return false;
    }

    output = static_cast<int>(value->numberValue);
    return true;
}

bool readBoolField(const JsonValue& object, const std::string& key, bool& output) {
    const JsonValue* value = object.get(key);
    if (value == nullptr || !value->isBool()) {
        return false;
    }

    output = value->boolValue;
    return true;
}
