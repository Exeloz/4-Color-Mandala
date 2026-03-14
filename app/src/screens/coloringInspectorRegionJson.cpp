#include "coloringInspector.h"

#include <cctype>
#include <regex>
#include <sstream>

namespace {
bool findMatchingClosingBrace(const std::string& content, size_t openingBraceIndex, size_t& closingBraceIndex) {
    if (openingBraceIndex >= content.size() || content[openingBraceIndex] != '{') {
        return false;
    }

    int depth = 0;
    bool inString = false;
    bool escaped = false;

    for (size_t i = openingBraceIndex; i < content.size(); ++i) {
        const char ch = content[i];

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                inString = false;
            }
            continue;
        }

        if (ch == '"') {
            inString = true;
            continue;
        }

        if (ch == '{') {
            ++depth;
            continue;
        }

        if (ch == '}') {
            --depth;
            if (depth == 0) {
                closingBraceIndex = i;
                return true;
            }
            if (depth < 0) {
                return false;
            }
        }
    }

    return false;
}

bool findMatchingClosingBracket(const std::string& content, size_t openingBracketIndex, size_t& closingBracketIndex) {
    if (openingBracketIndex >= content.size() || content[openingBracketIndex] != '[') {
        return false;
    }

    int depth = 0;
    bool inString = false;
    bool escaped = false;

    for (size_t i = openingBracketIndex; i < content.size(); ++i) {
        const char ch = content[i];

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                inString = false;
            }
            continue;
        }

        if (ch == '"') {
            inString = true;
            continue;
        }

        if (ch == '[') {
            ++depth;
            continue;
        }

        if (ch == ']') {
            --depth;
            if (depth == 0) {
                closingBracketIndex = i;
                return true;
            }
            if (depth < 0) {
                return false;
            }
        }
    }

    return false;
}

bool findRegionObjectRangeById(const std::string& content,
                               int regionId,
                               size_t& outObjectStart,
                               size_t& outObjectEnd) {
    const std::regex regionsKeyRegex(R"("regions"\s*:)");
    std::smatch regionsKeyMatch;
    if (!std::regex_search(content, regionsKeyMatch, regionsKeyRegex)) {
        return false;
    }

    const size_t regionsKeyPos = static_cast<size_t>(regionsKeyMatch.position(0));
    const size_t arrayStart = content.find('[', regionsKeyPos);
    if (arrayStart == std::string::npos) {
        return false;
    }

    size_t arrayEnd = std::string::npos;
    if (!findMatchingClosingBracket(content, arrayStart, arrayEnd)) {
        return false;
    }

    const std::regex idRegex(std::string("\\\"id\\\"\\s*:\\s*") + std::to_string(regionId) + "(?!\\d)");

    size_t cursor = arrayStart + 1;
    while (cursor < arrayEnd) {
        const size_t objectStart = content.find('{', cursor);
        if (objectStart == std::string::npos || objectStart >= arrayEnd) {
            break;
        }

        size_t objectEnd = std::string::npos;
        if (!findMatchingClosingBrace(content, objectStart, objectEnd) || objectEnd > arrayEnd) {
            return false;
        }

        const std::string objectText = content.substr(objectStart, objectEnd - objectStart + 1);
        if (objectText.find("\"points\"") != std::string::npos && std::regex_search(objectText, idRegex)) {
            outObjectStart = objectStart;
            outObjectEnd = objectEnd;
            return true;
        }

        cursor = objectEnd + 1;
    }

    return false;
}

bool applyRegionBlackoutToObjectText(std::string& regionObjectText) {
    if (regionObjectText.empty() || regionObjectText.front() != '{' || regionObjectText.back() != '}') {
        return false;
    }

    std::string updated = regionObjectText;
    const std::regex colorableRegex(R"("colorable"\s*:\s*(true|false))");
    if (std::regex_search(updated, colorableRegex)) {
        updated = std::regex_replace(updated, colorableRegex, "\"colorable\": false");
    }

    const std::regex defaultColorRegex(R"("defaultColor"\s*:\s*("[^"]*"|\[[^\]]*\]|\{[^{}]*\}|-?\d+(\.\d+)?|true|false|null))");
    if (std::regex_search(updated, defaultColorRegex)) {
        updated = std::regex_replace(updated, defaultColorRegex, "\"defaultColor\": \"black\"");
    }

    const bool hasColorable = std::regex_search(updated, std::regex(R"("colorable"\s*:)") );
    const bool hasDefaultColor = std::regex_search(updated, std::regex(R"("defaultColor"\s*:)") );
    if (hasColorable && hasDefaultColor) {
        regionObjectText = std::move(updated);
        return true;
    }

    size_t closeBrace = updated.rfind('}');
    if (closeBrace == std::string::npos) {
        return false;
    }

    size_t closeLineStart = updated.rfind('\n', closeBrace);
    if (closeLineStart == std::string::npos) {
        closeLineStart = 0;
    } else {
        ++closeLineStart;
    }

    std::string closeIndent;
    while (closeLineStart < updated.size()
           && (updated[closeLineStart] == ' ' || updated[closeLineStart] == '\t')) {
        closeIndent.push_back(updated[closeLineStart]);
        ++closeLineStart;
    }

    const std::string fieldIndent = closeIndent + "  ";

    size_t tail = closeBrace;
    while (tail > 0 && std::isspace(static_cast<unsigned char>(updated[tail - 1])) != 0) {
        --tail;
    }

    const bool objectHasEntries = tail > 0 && updated[tail - 1] != '{';
    std::ostringstream insert;
    if (objectHasEntries && updated[tail - 1] != ',') {
        insert << ",";
    }

    if (!hasColorable) {
        insert << "\n" << fieldIndent << "\"colorable\": false";
        if (!hasDefaultColor) {
            insert << ",";
        }
    }

    if (!hasDefaultColor) {
        insert << "\n" << fieldIndent << "\"defaultColor\": \"black\"";
    }

    insert << "\n" << closeIndent;
    updated.insert(closeBrace, insert.str());

    regionObjectText = std::move(updated);
    return true;
}
}

bool ColoringInspector::applyRegionBlackoutJsonEdit(const Mandala& mandala, int regionId) const {
    const std::string& regionsPath = mandala.getRegionsSourcePath();
    if (regionsPath.empty()) {
        return false;
    }

    std::string content;
    if (!readFileText(regionsPath, content)) {
        return false;
    }

    size_t objectStart = std::string::npos;
    size_t objectEnd = std::string::npos;
    if (!findRegionObjectRangeById(content, regionId, objectStart, objectEnd)) {
        return false;
    }

    std::string regionObject = content.substr(objectStart, objectEnd - objectStart + 1);
    if (!applyRegionBlackoutToObjectText(regionObject)) {
        return false;
    }

    content.replace(objectStart, objectEnd - objectStart + 1, regionObject);
    return writeFileText(regionsPath, content);
}
