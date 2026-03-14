#pragma once

#include "../mandala/mandala.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
    std::unordered_map<std::string, std::unique_ptr<JsonValue>> objectValue;

    bool isObject() const { return type == Type::Object; }
    bool isArray() const { return type == Type::Array; }
    bool isString() const { return type == Type::String; }
    bool isNumber() const { return type == Type::Number; }
    bool isBool() const { return type == Type::Bool; }

    const JsonValue* get(const std::string& key) const;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& input);
    JsonValue parseRoot();

private:
    const std::string& text;
    size_t position;

    void skipWhitespace();
    bool tryConsume(char expectedChar);
    void expect(char expectedChar);
    JsonValue parseValue();
    JsonValue parseObject();
    JsonValue parseArray();
    std::string parseString();
    JsonValue parseNumber();
    JsonValue parseBool();
    JsonValue parseNull();
};

bool tryLoadTextFile(const std::vector<std::string>& candidatePaths,
                     std::string& outputText,
                     std::string& loadedPath);

std::vector<std::string> makeCandidateAssetPaths(const std::string& relativePath);

bool readStringField(const JsonValue& object, const std::string& key, std::string& output);
bool readIntField(const JsonValue& object, const std::string& key, int& output);
bool readBoolField(const JsonValue& object, const std::string& key, bool& output);
bool parseColorField(const JsonValue& value, Color& output);
bool parseRegionVertices(const JsonValue& pointsValue, std::vector<Vector2>& vertices);
