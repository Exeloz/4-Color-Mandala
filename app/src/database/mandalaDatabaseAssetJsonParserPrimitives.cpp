#include "mandalaDatabaseAssetJson.h"

#include <cctype>
#include <stdexcept>

std::string JsonParser::parseString() {
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

JsonValue JsonParser::parseNumber() {
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

JsonValue JsonParser::parseBool() {
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

JsonValue JsonParser::parseNull() {
    if (text.compare(position, 4, "null") != 0) {
        throw std::runtime_error("Invalid null literal");
    }

    position += 4;
    JsonValue nullValue;
    nullValue.type = JsonValue::Type::Null;
    return nullValue;
}
