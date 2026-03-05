#include "coloringInspector.h"
#include "../ui/colors.h"
#include "../ui/input.h"
#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

namespace {
Color oppositeColor(Color color) {
    return {
        static_cast<unsigned char>(255 - color.r),
        static_cast<unsigned char>(255 - color.g),
        static_cast<unsigned char>(255 - color.b),
        color.a
    };
}

FillPattern makeLargeDottedStyle(Color fillColor) {
    FillPattern style;
    style.type = FillPatternType::Dotted;
    style.size = 7.5f;
    style.useAccentColor = true;
    style.accentColor = oppositeColor(fillColor);
    return style;
}

FillPattern makeLargeStripedStyle(Color fillColor) {
    FillPattern style;
    style.type = FillPatternType::Striped;
    style.size = 5.f;
    style.useAccentColor = true;
    style.accentColor = oppositeColor(fillColor);
    return style;
}

bool isNativeMobilePlatform() {
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    return true;
#else
    return false;
#endif
}

Color resolveRegionPaletteColor(const Region& region, const std::vector<Color>& colorPalette, Color fallbackColor) {
    int colorIndex = region.getColor();
    if (region.hasColor() && colorIndex >= 0 && colorIndex < static_cast<int>(colorPalette.size())) {
        return colorPalette[colorIndex];
    }

    return fallbackColor;
}

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
    const std::regex regionsKeyRegex(R"("regions"\s*:)" );
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

    const bool hasColorable = std::regex_search(updated, std::regex(R"("colorable"\s*:)"));
    const bool hasDefaultColor = std::regex_search(updated, std::regex(R"("defaultColor"\s*:)"));
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
    while (closeLineStart < updated.size() && (updated[closeLineStart] == ' ' || updated[closeLineStart] == '\t')) {
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

void ColoringInspector::validateAdjacency(const Mandala& mandala) {
    validationInspector.validateAdjacency(mandala);
}

void ColoringInspector::drawValidationOverlay(const Mandala& mandala, float cameraZoom) const {
    validationInspector.drawValidationOverlay(mandala, cameraZoom);
}

void ColoringInspector::enterAnalysisMode() {
    analysisMode = true;
    clearAnalysisSelection();
}

void ColoringInspector::exitAnalysisMode() {
    analysisMode = false;
    clearAnalysisSelection();
}

void ColoringInspector::clearAnalysisSelection() {
    analysisInspectRegionId = -1;
    analysisHoverRegionId = -1;
}

bool ColoringInspector::isAnalysisMode() const {
    return analysisMode;
}

void ColoringInspector::updateAnalysis(const Mandala& mandala, const Camera2D& camera, bool pointerOverUi, bool isDraggingCamera) {
    if (!analysisMode) {
        return;
    }

    if (isNativeMobilePlatform()) {
        analysisHoverRegionId = -1;

        if (pointerOverUi) {
            return;
        }

        if (!isDraggingCamera && Input::IsPointerPressed()) {
            Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
            int tappedRegionId = getRegionIdAtWorldPosition(mandala, worldPos);
            if (tappedRegionId >= 0) {
                analysisInspectRegionId = tappedRegionId;
            }
        }
        return;
    }

    if (pointerOverUi) {
        analysisHoverRegionId = -1;
        return;
    }

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    analysisHoverRegionId = getRegionIdAtWorldPosition(mandala, worldPos);

    if (!isDraggingCamera && Input::IsPointerPressed() && analysisHoverRegionId >= 0) {
        analysisInspectRegionId = analysisHoverRegionId;
    }
}

void ColoringInspector::updateDebug(Mandala& mandala, const Camera2D& camera, Camera2D& mutableCamera) {
    int previousInspectRegionId = debugInspectRegionId;

    if (IsKeyPressed(KEY_F3)) {
        debugAdjacencyMode = !debugAdjacencyMode;
        if (!debugAdjacencyMode) {
            debugInspectRegionId = -1;
            debugHoverRegionId = -1;
        }
    }

    if (!debugAdjacencyMode) {
        return;
    }

    int arrowDirection = 0;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP)) {
        arrowDirection = 1;
    } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_DOWN)) {
        arrowDirection = -1;
    }

    if (arrowDirection != 0) {
        const auto& regions = mandala.getRegions();
        std::vector<int> sortedIds = collectSortedRegionIds(regions);
        debugInspectRegionId = cycleRegionId(sortedIds, debugInspectRegionId, arrowDirection);
    }

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    debugHoverRegionId = getRegionIdAtWorldPosition(mandala, worldPos);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && debugHoverRegionId >= 0) {
        debugInspectRegionId = debugHoverRegionId;
    }

    if (IsKeyPressed(KEY_C)) {
        debugInspectRegionId = -1;
    }

    if (debugInspectRegionId >= 0 && debugHoverRegionId >= 0 && debugInspectRegionId != debugHoverRegionId) {
        if (IsKeyPressed(KEY_A)) {
            logAdjacencySuggestion(mandala, true, debugInspectRegionId, debugHoverRegionId);
        }
        if (IsKeyPressed(KEY_R)) {
            logAdjacencySuggestion(mandala, false, debugInspectRegionId, debugHoverRegionId);
        }
    }

    if (IsKeyPressed(KEY_B)) {
        int targetRegionId = debugInspectRegionId >= 0 ? debugInspectRegionId : debugHoverRegionId;
        if (targetRegionId >= 0) {
            logRegionBlackoutSuggestion(mandala, targetRegionId);
        }
    }

    if (debugInspectRegionId >= 0 && debugInspectRegionId != previousInspectRegionId) {
        centerCameraOnRegion(mandala, debugInspectRegionId, mutableCamera);
    }
}

void ColoringInspector::drawAnalysisOverlay(const Mandala& mandala, const std::vector<Color>& colorPalette) const {
    if (!analysisMode) {
        return;
    }

    if (analysisInspectRegionId >= 0) {
        const Region* selectedRegion = mandala.getRegionById(analysisInspectRegionId);
        if (selectedRegion != nullptr) {
            Color selectedFill = resolveRegionPaletteColor(*selectedRegion, colorPalette, selectedRegion->getDefaultColor());
            selectedRegion->drawWithColor(selectedFill, Colors::DarkCyan, 6.0f,
                                          makeLargeStripedStyle(selectedFill));

            const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(analysisInspectRegionId);
            for (int neighborId : neighbors) {
                const Region* neighborRegion = mandala.getRegionById(neighborId);
                if (neighborRegion != nullptr) {
                    Color neighborFill = resolveRegionPaletteColor(*neighborRegion, colorPalette, neighborRegion->getDefaultColor());
                    neighborRegion->drawWithColor(neighborFill, Colors::Blue, 3.0f,
                                                  makeLargeDottedStyle(neighborFill));
                }
            }
        }
    }

    if (analysisHoverRegionId >= 0 && analysisHoverRegionId != analysisInspectRegionId) {
        const Region* hoverRegion = mandala.getRegionById(analysisHoverRegionId);
        if (hoverRegion != nullptr) {
            Color hoverFill = Fade(Colors::LightSkyBlue, 0.35f);
            hoverRegion->drawWithColor(hoverFill, Colors::DodgerBlue, 2.0f,
                                       makeLargeStripedStyle(hoverFill));
        }
    }
}

void ColoringInspector::drawDebugOverlay(const Mandala& mandala) const {
    if (!debugAdjacencyMode) {
        return;
    }

    if (debugInspectRegionId < 0) {
        return;
    }

    const Region* selectedRegion = mandala.getRegionById(debugInspectRegionId);
    if (selectedRegion == nullptr) {
        return;
    }

    selectedRegion->drawWithColor(Fade(Colors::Cyan, 0.45f), Colors::DarkCyan, 6.0f);

    const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
    for (int neighborId : neighbors) {
        const Region* neighborRegion = mandala.getRegionById(neighborId);
        if (neighborRegion == nullptr) {
            continue;
        }

        neighborRegion->drawWithColor(Fade(Colors::Blue, 0.30f), Colors::Blue, 4.0f);
    }

    if (debugHoverRegionId >= 0 && debugHoverRegionId != debugInspectRegionId) {
        const Region* hoverRegion = mandala.getRegionById(debugHoverRegionId);
        if (hoverRegion != nullptr) {
            hoverRegion->drawWithColor(Fade(Colors::LightSkyBlue, 0.35f), Colors::DodgerBlue, 2.0f);
        }
    }
}

void ColoringInspector::drawDebugInfoPanel(const Mandala& mandala, float uiScale) const {
    if (!debugAdjacencyMode) {
        return;
    }

    int infoX = static_cast<int>(15.0f * uiScale);
    int infoY = static_cast<int>(70.0f * uiScale);
    int infoW = static_cast<int>(760.0f * uiScale);
    int infoH = static_cast<int>(58.0f * uiScale);
    DrawRectangle(infoX, infoY, infoW, infoH, Fade(Colors::White, 0.85f));
    DrawRectangleLines(infoX, infoY, infoW, infoH, Colors::DarkGray);

    std::ostringstream info;
    info << "DEBUG ADJ: ON  |  Hover: " << debugHoverRegionId
         << "  |  Inspect (Right Click): " << debugInspectRegionId
            << "  |  Clear Inspect: C  |  A=Add  R=Remove  B=Black+Lock";
    DrawText(info.str().c_str(), static_cast<int>(24.0f * uiScale), static_cast<int>(82.0f * uiScale),
             static_cast<int>(20.0f * uiScale), Colors::Black);

    if (debugInspectRegionId >= 0) {
        const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
        std::ostringstream neighborText;
        neighborText << "Neighbors(" << neighbors.size() << "): ";

        bool first = true;
        for (int id : neighbors) {
            if (!first) {
                neighborText << ", ";
            }
            neighborText << id;
            first = false;
        }

        DrawText(neighborText.str().c_str(), static_cast<int>(24.0f * uiScale), static_cast<int>(103.0f * uiScale),
                 static_cast<int>(18.0f * uiScale), Colors::DarkBlue);
    }
}

void ColoringInspector::logRegionBlackoutSuggestion(Mandala& mandala, int regionId) {
    TraceLog(LOG_INFO, "[REGION DEBUG] Set region %d defaultColor=black and colorable=false", regionId);

    Region* region = mandala.getRegionById(regionId);
    if (region != nullptr) {
        region->setDefaultColor(Colors::Black);
        region->setColor(-1);
        region->setColorable(false);
        TraceLog(LOG_INFO, "[REGION DEBUG] Applied immediate in-memory blackout for region %d.", regionId);
    } else {
        TraceLog(LOG_WARNING, "[REGION DEBUG] Region %d not found in current mandala instance.", regionId);
    }

    if (applyRegionBlackoutJsonEdit(mandala, regionId)) {
        TraceLog(LOG_INFO, "[REGION DEBUG] Updated regions JSON on disk for mandala %d.", mandala.getId());
    } else {
        TraceLog(LOG_WARNING, "[REGION DEBUG] Failed to update regions JSON on disk for mandala %d.", mandala.getId());
    }

    if (applyAdjacencyJsonRemoveAllForRegion(mandala, regionId)) {
        TraceLog(LOG_INFO, "[REGION DEBUG] Removed all adjacency pairs containing region %d in JSON.", regionId);
    } else {
        TraceLog(LOG_WARNING, "[REGION DEBUG] Failed to clear adjacency JSON pairs for region %d.", regionId);
    }
}

bool ColoringInspector::isDebugAdjacencyMode() const {
    return debugAdjacencyMode;
}

int ColoringInspector::getDebugInspectRegionId() const {
    return debugInspectRegionId;
}

int ColoringInspector::getDebugHoverRegionId() const {
    return debugHoverRegionId;
}

std::vector<int> ColoringInspector::collectSortedRegionIds(const std::vector<Region>& regions) {
    std::vector<int> ids;
    ids.reserve(regions.size());
    for (const auto& region : regions) {
        if (region.isColorable()) {
            ids.push_back(region.getId());
        }
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

int ColoringInspector::cycleRegionId(const std::vector<int>& sortedIds, int currentId, int direction) {
    if (sortedIds.empty()) {
        return -1;
    }

    if (currentId < 0) {
        return sortedIds.front();
    }

    auto it = std::lower_bound(sortedIds.begin(), sortedIds.end(), currentId);
    if (it == sortedIds.end() || *it != currentId) {
        if (direction > 0) {
            return (it == sortedIds.end()) ? sortedIds.front() : *it;
        }

        if (it == sortedIds.begin()) {
            return sortedIds.back();
        }

        --it;
        return *it;
    }

    int index = static_cast<int>(it - sortedIds.begin());
    int size = static_cast<int>(sortedIds.size());
    int nextIndex = (index + direction + size) % size;
    return sortedIds[nextIndex];
}

int ColoringInspector::getRegionIdAtWorldPosition(const Mandala& mandala, Vector2 worldPos) const {
    const auto& regions = mandala.getRegions();
    for (auto it = regions.rbegin(); it != regions.rend(); ++it) {
        if (it->isPointInRegion(worldPos) && it->isColorable()) {
            return it->getId();
        }
    }

    return -1;
}

void ColoringInspector::centerCameraOnRegion(const Mandala& mandala, int regionId, Camera2D& camera) const {
    const Region* region = mandala.getRegionById(regionId);
    if (region == nullptr) {
        return;
    }

    camera.target = region->getCentroid();
}

void ColoringInspector::logAdjacencySuggestion(const Mandala& mandala, bool shouldExist, int regionA, int regionB) {
    int a = std::min(regionA, regionB);
    int b = std::max(regionA, regionB);

    bool currentlyAdjacent = mandala.getAdjacencyGraph().areAdjacent(a, b);

    if (shouldExist) {
        TraceLog(LOG_INFO, "[ADJ DEBUG] Suggest ADD (%d, %d)", a, b);
        if (currentlyAdjacent) {
            TraceLog(LOG_INFO, "[ADJ DEBUG] Already adjacent in current graph.");
        }
        TraceLog(LOG_INFO, "[ADJ DEBUG] Line to add: adjacencyGraph.addAdjacency(%d, %d);", a, b);
        if (applyAdjacencyJsonEdit(mandala, true, a, b)) {
            TraceLog(LOG_INFO, "[ADJ DEBUG] Updated adjacency JSON on disk for mandala %d.", mandala.getId());
        } else {
            TraceLog(LOG_WARNING, "[ADJ DEBUG] Failed to update adjacency JSON on disk for mandala %d.", mandala.getId());
        }
        return;
    }

    TraceLog(LOG_INFO, "[ADJ DEBUG] Suggest REMOVE (%d, %d)", a, b);
    if (!currentlyAdjacent) {
        TraceLog(LOG_INFO, "[ADJ DEBUG] Pair not currently adjacent in graph.");
    }
    TraceLog(LOG_INFO, "[ADJ DEBUG] Remove pair from adjacency JSON: [%d, %d]", a, b);
    if (applyAdjacencyJsonEdit(mandala, false, a, b)) {
        TraceLog(LOG_INFO, "[ADJ DEBUG] Updated adjacency JSON on disk for mandala %d.", mandala.getId());
    } else {
        TraceLog(LOG_WARNING, "[ADJ DEBUG] Failed to update adjacency JSON on disk for mandala %d.", mandala.getId());
    }
}

bool ColoringInspector::applyAdjacencyJsonEdit(const Mandala& mandala,
                                               bool shouldExist,
                                               int regionA,
                                               int regionB) const {
    const std::string& adjacencyPath = mandala.getAdjacencySourcePath();
    if (adjacencyPath.empty()) {
        return false;
    }

    std::string content;
    if (!readFileText(adjacencyPath, content)) {
        return false;
    }

    std::set<std::pair<int, int>> pairs;
    const std::regex pairRegex(R"(\[\s*(-?\d+)\s*,\s*(-?\d+)\s*\])");
    for (std::sregex_iterator it(content.begin(), content.end(), pairRegex), end; it != end; ++it) {
        const int first = std::stoi((*it)[1].str());
        const int second = std::stoi((*it)[2].str());
        if (first == second) {
            continue;
        }
        pairs.emplace(std::min(first, second), std::max(first, second));
    }

    const std::pair<int, int> targetPair{std::min(regionA, regionB), std::max(regionA, regionB)};
    if (shouldExist) {
        pairs.insert(targetPair);
    } else {
        pairs.erase(targetPair);
    }

    std::ostringstream output;
    output << "{\n";
    output << "  \"mandala_id\": " << mandala.getId() << ",\n";
    output << "  \"pairs\": [\n";

    bool first = true;
    for (const auto& pair : pairs) {
        if (!first) {
            output << ",\n";
        }
        output << "    [" << pair.first << ", " << pair.second << "]";
        first = false;
    }

    output << "\n  ]\n";
    output << "}\n";

    return writeFileText(adjacencyPath, output.str());
}

bool ColoringInspector::applyAdjacencyJsonRemoveAllForRegion(const Mandala& mandala, int regionId) const {
    const std::string& adjacencyPath = mandala.getAdjacencySourcePath();
    if (adjacencyPath.empty()) {
        return false;
    }

    std::string content;
    if (!readFileText(adjacencyPath, content)) {
        return false;
    }

    std::set<std::pair<int, int>> pairs;
    const std::regex pairRegex(R"(\[\s*(-?\d+)\s*,\s*(-?\d+)\s*\])");
    for (std::sregex_iterator it(content.begin(), content.end(), pairRegex), end; it != end; ++it) {
        const int first = std::stoi((*it)[1].str());
        const int second = std::stoi((*it)[2].str());
        if (first == second) {
            continue;
        }
        if (first == regionId || second == regionId) {
            continue;
        }
        pairs.emplace(std::min(first, second), std::max(first, second));
    }

    std::ostringstream output;
    output << "{\n";
    output << "  \"mandala_id\": " << mandala.getId() << ",\n";
    output << "  \"pairs\": [\n";

    bool first = true;
    for (const auto& pair : pairs) {
        if (!first) {
            output << ",\n";
        }
        output << "    [" << pair.first << ", " << pair.second << "]";
        first = false;
    }

    output << "\n  ]\n";
    output << "}\n";

    return writeFileText(adjacencyPath, output.str());
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

bool ColoringInspector::readFileText(const std::string& path, std::string& content) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    content = buffer.str();
    return true;
}

bool ColoringInspector::writeFileText(const std::string& path, const std::string& content) {
    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }

    stream << content;
    return stream.good();
}
