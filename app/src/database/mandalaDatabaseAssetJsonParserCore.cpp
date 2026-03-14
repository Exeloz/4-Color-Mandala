#include "mandalaDatabaseAssetJson.h"

#include <cctype>
#include <memory>
#include <stdexcept>
#include <utility>

const JsonValue* JsonValue::get(const std::string& key) const {
    if (!isObject()) {
        return nullptr;
    }

    auto iterator = objectValue.find(key);
    if (iterator == objectValue.end()) {
        return nullptr;
    }
    return iterator->second.get();
}

JsonParser::JsonParser(const std::string& input)
    : text(input), position(0) {}

JsonValue JsonParser::parseRoot() {
    skipWhitespace();
    JsonValue root = parseValue();
    skipWhitespace();
    if (position != text.size()) {
        throw std::runtime_error("Unexpected content after JSON root");
    }
    return root;
}

void JsonParser::skipWhitespace() {
    while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position])) != 0) {
        ++position;
    }
}

bool JsonParser::tryConsume(char expectedChar) {
    if (position < text.size() && text[position] == expectedChar) {
        ++position;
        return true;
    }
    return false;
}

void JsonParser::expect(char expectedChar) {
    if (!tryConsume(expectedChar)) {
        throw std::runtime_error(std::string("Expected character: ") + expectedChar);
    }
}

JsonValue JsonParser::parseValue() {
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

JsonValue JsonParser::parseObject() {
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
        object.objectValue.emplace(std::move(key), std::make_unique<JsonValue>(std::move(value)));

        skipWhitespace();
        if (tryConsume('}')) {
            break;
        }
        expect(',');
    }

    return object;
}

JsonValue JsonParser::parseArray() {
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
