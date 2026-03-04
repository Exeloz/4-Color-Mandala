#include "mandalaDatabase.h"
#include "../ui/colors.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <raylib.h>

namespace {
    constexpr float SCREEN_CENTER_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
    constexpr float DEGREES_TO_RADIANS = 3.14159f / 180.0f;
    
    constexpr float HEXAGON_RADIUS = 3000.0f;
    constexpr float HEXAGON_INNER_RATIO = 0.5f;
    constexpr int HEXAGON_SEGMENTS = 6;
    constexpr float HEXAGON_DEGREES_PER_SEGMENT = 360.0f / HEXAGON_SEGMENTS;

    struct JsonValue {
        enum class Type {
            Null,
            Bool,
            Number,
            String,
            Array,
            Object,
        };

        Type type = Type::Null;
        bool boolValue = false;
        double numberValue = 0.0;
        std::string stringValue;
        std::vector<JsonValue> arrayValue;
        std::unordered_map<std::string, JsonValue> objectValue;

        bool isObject() const { return type == Type::Object; }
        bool isArray() const { return type == Type::Array; }
        bool isString() const { return type == Type::String; }
        bool isNumber() const { return type == Type::Number; }
        bool isBool() const { return type == Type::Bool; }

        const JsonValue* get(const std::string& key) const {
            if (!isObject()) {
                return nullptr;
            }

            auto iterator = objectValue.find(key);
            if (iterator == objectValue.end()) {
                return nullptr;
            }
            return &iterator->second;
        }
    };

    class JsonParser {
    public:
        explicit JsonParser(const std::string& input)
            : text(input), position(0) {}

        JsonValue parseRoot() {
            skipWhitespace();
            JsonValue root = parseValue();
            skipWhitespace();
            if (position != text.size()) {
                throw std::runtime_error("Unexpected content after JSON root");
            }
            return root;
        }

    private:
        const std::string& text;
        size_t position;

        void skipWhitespace() {
            while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position])) != 0) {
                ++position;
            }
        }

        bool tryConsume(char expectedChar) {
            if (position < text.size() && text[position] == expectedChar) {
                ++position;
                return true;
            }
            return false;
        }

        void expect(char expectedChar) {
            if (!tryConsume(expectedChar)) {
                throw std::runtime_error(std::string("Expected character: ") + expectedChar);
            }
        }

        JsonValue parseValue() {
            skipWhitespace();

            if (position >= text.size()) {
                throw std::runtime_error("Unexpected end of JSON");
            }

            const char current = text[position];
            if (current == '{') {
                return parseObject();
            }
            if (current == '[') {
                return parseArray();
            }
            if (current == '"') {
                JsonValue stringValue;
                stringValue.type = JsonValue::Type::String;
                stringValue.stringValue = parseString();
                return stringValue;
            }
            if (current == 't' || current == 'f') {
                return parseBool();
            }
            if (current == 'n') {
                return parseNull();
            }
            if (current == '-' || std::isdigit(static_cast<unsigned char>(current)) != 0) {
                return parseNumber();
            }

            throw std::runtime_error("Unexpected character while parsing JSON value");
        }

        JsonValue parseObject() {
            JsonValue object;
            object.type = JsonValue::Type::Object;

            expect('{');
            skipWhitespace();

            if (tryConsume('}')) {
                return object;
            }

            while (true) {
                skipWhitespace();
                if (position >= text.size() || text[position] != '"') {
                    throw std::runtime_error("Expected string key in object");
                }

                std::string key = parseString();
                skipWhitespace();
                expect(':');
                JsonValue value = parseValue();
                object.objectValue.emplace(std::move(key), std::move(value));

                skipWhitespace();
                if (tryConsume('}')) {
                    break;
                }
                expect(',');
            }

            return object;
        }

        JsonValue parseArray() {
            JsonValue array;
            array.type = JsonValue::Type::Array;

            expect('[');
            skipWhitespace();
            if (tryConsume(']')) {
                return array;
            }

            while (true) {
                array.arrayValue.push_back(parseValue());
                skipWhitespace();

                if (tryConsume(']')) {
                    break;
                }
                expect(',');
            }

            return array;
        }

        std::string parseString() {
            expect('"');
            std::string result;

            while (position < text.size()) {
                const char character = text[position++];
                if (character == '"') {
                    return result;
                }

                if (character == '\\') {
                    if (position >= text.size()) {
                        throw std::runtime_error("Invalid escape sequence");
                    }

                    const char escaped = text[position++];
                    switch (escaped) {
                        case '"': result.push_back('"'); break;
                        case '\\': result.push_back('\\'); break;
                        case '/': result.push_back('/'); break;
                        case 'b': result.push_back('\b'); break;
                        case 'f': result.push_back('\f'); break;
                        case 'n': result.push_back('\n'); break;
                        case 'r': result.push_back('\r'); break;
                        case 't': result.push_back('\t'); break;
                        default:
                            throw std::runtime_error("Unsupported escape sequence in JSON string");
                    }
                    continue;
                }

                result.push_back(character);
            }

            throw std::runtime_error("Unterminated JSON string");
        }

        JsonValue parseNumber() {
            const size_t numberStart = position;

            if (tryConsume('-')) {
            }

            while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position])) != 0) {
                ++position;
            }

            if (position < text.size() && text[position] == '.') {
                ++position;
                while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position])) != 0) {
                    ++position;
                }
            }

            if (position < text.size() && (text[position] == 'e' || text[position] == 'E')) {
                ++position;
                if (position < text.size() && (text[position] == '+' || text[position] == '-')) {
                    ++position;
                }
                while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position])) != 0) {
                    ++position;
                }
            }

            JsonValue number;
            number.type = JsonValue::Type::Number;
            number.numberValue = std::stod(text.substr(numberStart, position - numberStart));
            return number;
        }

        JsonValue parseBool() {
            JsonValue boolean;
            boolean.type = JsonValue::Type::Bool;

            if (text.compare(position, 4, "true") == 0) {
                boolean.boolValue = true;
                position += 4;
                return boolean;
            }
            if (text.compare(position, 5, "false") == 0) {
                boolean.boolValue = false;
                position += 5;
                return boolean;
            }

            throw std::runtime_error("Invalid boolean literal");
        }

        JsonValue parseNull() {
            if (text.compare(position, 4, "null") != 0) {
                throw std::runtime_error("Invalid null literal");
            }

            position += 4;
            JsonValue nullValue;
            nullValue.type = JsonValue::Type::Null;
            return nullValue;
        }
    };

    bool tryLoadTextFile(const std::vector<std::string>& candidatePaths, std::string& outputText, std::string& loadedPath) {
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

    bool parseColorFromString(const std::string& value, Color& output) {
        std::string normalized;
        normalized.reserve(value.size());
        for (char c : value) {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        if (normalized == "black") {
            output = Colors::Black;
            return true;
        }
        if (normalized == "white") {
            output = Colors::White;
            return true;
        }
        if (normalized == "transparent") {
            output = Colors::Transparent;
            return true;
        }
        if (normalized == "none") {
            output = Colors::None;
            return true;
        }

        return false;
    }

    bool toColorChannel(const JsonValue& value, unsigned char& channel) {
        if (!value.isNumber()) {
            return false;
        }

        const double raw = value.numberValue;
        if (raw < 0.0 || raw > 255.0) {
            return false;
        }

        channel = static_cast<unsigned char>(raw);
        return true;
    }

    bool parseColorField(const JsonValue& value, Color& output) {
        if (value.isString()) {
            return parseColorFromString(value.stringValue, output);
        }

        if (value.isArray()) {
            if (value.arrayValue.size() < 3) {
                return false;
            }

            unsigned char r = 0;
            unsigned char g = 0;
            unsigned char b = 0;
            unsigned char a = 255;

            if (!toColorChannel(value.arrayValue[0], r)
                || !toColorChannel(value.arrayValue[1], g)
                || !toColorChannel(value.arrayValue[2], b)) {
                return false;
            }

            if (value.arrayValue.size() >= 4 && !toColorChannel(value.arrayValue[3], a)) {
                return false;
            }

            output = Color{r, g, b, a};
            return true;
        }

        if (value.isObject()) {
            const JsonValue* red = value.get("r");
            const JsonValue* green = value.get("g");
            const JsonValue* blue = value.get("b");
            const JsonValue* alpha = value.get("a");
            if (red == nullptr || green == nullptr || blue == nullptr) {
                return false;
            }

            unsigned char r = 0;
            unsigned char g = 0;
            unsigned char b = 0;
            unsigned char a = 255;
            if (!toColorChannel(*red, r) || !toColorChannel(*green, g) || !toColorChannel(*blue, b)) {
                return false;
            }
            if (alpha != nullptr && !toColorChannel(*alpha, a)) {
                return false;
            }

            output = Color{r, g, b, a};
            return true;
        }

        return false;
    }

    bool arePointsNear(const Vector2& a, const Vector2& b, float epsilon = 0.0001f) {
        return std::fabs(a.x - b.x) <= epsilon && std::fabs(a.y - b.y) <= epsilon;
    }

    float computeSignedArea(const std::vector<Vector2>& vertices) {
        if (vertices.size() < 3) {
            return 0.0f;
        }

        float areaTimesTwo = 0.0f;
        for (size_t i = 0; i < vertices.size(); ++i) {
            const Vector2& current = vertices[i];
            const Vector2& next = vertices[(i + 1) % vertices.size()];
            areaTimesTwo += current.x * next.y - next.x * current.y;
        }
        return areaTimesTwo * 0.5f;
    }

    void normalizePolygonForTessellation(std::vector<Vector2>& vertices) {
        if (vertices.empty()) {
            return;
        }

        std::vector<Vector2> cleaned;
        cleaned.reserve(vertices.size());
        for (const Vector2& vertex : vertices) {
            if (cleaned.empty() || !arePointsNear(cleaned.back(), vertex)) {
                cleaned.push_back(vertex);
            }
        }

        if (cleaned.size() >= 2 && arePointsNear(cleaned.front(), cleaned.back())) {
            cleaned.pop_back();
        }

        vertices = std::move(cleaned);
        if (vertices.size() < 3) {
            return;
        }

        if (computeSignedArea(vertices) > 0.0f) {
            std::reverse(vertices.begin(), vertices.end());
        }
    }

    bool parseRegionVertices(const JsonValue& pointsValue, std::vector<Vector2>& vertices) {
        if (!pointsValue.isArray()) {
            return false;
        }

        vertices.clear();
        vertices.reserve(pointsValue.arrayValue.size());

        for (const JsonValue& pointValue : pointsValue.arrayValue) {
            if (!pointValue.isArray() || pointValue.arrayValue.size() < 2) {
                return false;
            }

            const JsonValue& xValue = pointValue.arrayValue[0];
            const JsonValue& yValue = pointValue.arrayValue[1];
            if (!xValue.isNumber() || !yValue.isNumber()) {
                return false;
            }

            vertices.push_back(Vector2{
                static_cast<float>(xValue.numberValue),
                static_cast<float>(yValue.numberValue),
            });
        }

        normalizePolygonForTessellation(vertices);
        return vertices.size() >= 3;
    }
}

MandalaDatabase::MandalaDatabase() {
    createSampleMandala();
}

void MandalaDatabase::createSampleMandala() {
    createHexagonMandala();
    if (!loadManifest()) {
        TraceLog(LOG_WARNING, "Mandala manifest not loaded; only tutorial mandala is available.");
    }
}

void MandalaDatabase::loadMandala(int id) {
    if (hasMandala(id)) {
        return;
    }

    auto iterator = std::find_if(
        mandalaDescriptors.begin(),
        mandalaDescriptors.end(),
        [id](const MandalaAssetDescriptor& descriptor) {
            return descriptor.id == id;
        }
    );

    if (iterator == mandalaDescriptors.end()) {
        TraceLog(LOG_WARNING, "Mandala id %d not found in manifest descriptors", id);
        return;
    }

    if (!loadMandalaFromAssets(*iterator)) {
        TraceLog(LOG_WARNING, "Failed loading mandala id %d from assets", id);
    }
}

const std::vector<std::shared_ptr<Mandala>>& MandalaDatabase::getAllMandala() const {
    return mandalaList;
}

std::shared_ptr<Mandala> MandalaDatabase::getMandalaById(int id) const {
    for (const auto& mandala : mandalaList) {
        if (mandala->getId() == id) {
            return mandala;
        }
    }
    return nullptr;
}

void MandalaDatabase::createHexagonMandala() {
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph(HEXAGON_SEGMENTS);
    
    Vector2 center = {SCREEN_CENTER_X, SCREEN_CENTER_Y};

    for (int i = 0; i < HEXAGON_SEGMENTS; i++) {
        float angle1 = (i * HEXAGON_DEGREES_PER_SEGMENT) * DEGREES_TO_RADIANS;
        float angle2 = ((i + 1) * HEXAGON_DEGREES_PER_SEGMENT) * DEGREES_TO_RADIANS;

        std::vector<Vector2> vertices = {
            {center.x + HEXAGON_RADIUS * std::cos(angle1), center.y + HEXAGON_RADIUS * std::sin(angle1)},
            {center.x + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::cos(angle1), center.y + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::sin(angle1)},
            {center.x, center.y},
            {center.x + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::cos(angle2), center.y + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::sin(angle2)},
            {center.x + HEXAGON_RADIUS * std::cos(angle2), center.y + HEXAGON_RADIUS * std::sin(angle2)},
        };

        regions.emplace_back(i, vertices);
    }

    for (int i = 0; i < HEXAGON_SEGMENTS; i++) {
        adjacencyGraph.addAdjacency(i, (i + 1) % HEXAGON_SEGMENTS);
    }

    auto mandala = std::make_shared<Mandala>(0, "Tutorial", regions, adjacencyGraph);
    mandalaList.push_back(mandala);
    mandalaListItems.push_back({0, "Tutorial"});
}

bool MandalaDatabase::loadManifest() {
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

    mandalaDescriptors.clear();
    for (const JsonValue& entry : root.arrayValue) {
        if (!entry.isObject()) {
            continue;
        }

        MandalaAssetDescriptor descriptor{};
        if (!readIntField(entry, "id", descriptor.id)) {
            continue;
        }
        if (!readStringField(entry, "name", descriptor.name)) {
            continue;
        }
        if (!readStringField(entry, "regions", descriptor.regionsPath)) {
            continue;
        }
        if (!readStringField(entry, "adjacency", descriptor.adjacencyPath)) {
            continue;
        }

        mandalaDescriptors.push_back(descriptor);
        mandalaListItems.push_back({descriptor.id, descriptor.name});
    }

    return true;
}

const std::vector<MandalaDatabase::MandalaListItem>& MandalaDatabase::getMandalaListItems() const {
    return mandalaListItems;
}

bool MandalaDatabase::loadMandalaFromAssets(const MandalaAssetDescriptor& descriptor) {
    struct RawRegionData {
        int id = -1;
        std::vector<Vector2> vertices;
        bool hasColorable = false;
        bool colorable = true;
        bool hasDefaultColor = false;
        Color defaultColor = Colors::None;
    };

    std::string regionsText;
    std::string loadedRegionsPath;
    if (!tryLoadTextFile(makeCandidateAssetPaths(descriptor.regionsPath), regionsText, loadedRegionsPath)) {
        TraceLog(LOG_ERROR, "Unable to load regions file for mandala %d", descriptor.id);
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

    std::string adjacencyText;
    std::string loadedAdjacencyPath;
    if (!tryLoadTextFile(makeCandidateAssetPaths(descriptor.adjacencyPath), adjacencyText, loadedAdjacencyPath)) {
        TraceLog(LOG_ERROR, "Unable to load adjacency file for mandala %d", descriptor.id);
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

    mandalaList.push_back(std::make_shared<Mandala>(descriptor.id, descriptor.name, regions, adjacencyGraph));
    return true;
}

bool MandalaDatabase::hasMandala(int id) const {
    return std::any_of(
        mandalaList.begin(),
        mandalaList.end(),
        [id](const std::shared_ptr<Mandala>& mandala) {
            return mandala != nullptr && mandala->getId() == id;
        }
    );
}
