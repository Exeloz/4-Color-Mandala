#include "button.h"
#include "colors.h"
#include "input.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace {
std::string trimToEllipsis(const std::string& text, int textSize, float maxWidth) {
    const std::string ellipsis = "...";
    if (MeasureText(text.c_str(), textSize) <= maxWidth) {
        return text;
    }

    std::string candidate = text;
    while (!candidate.empty()) {
        candidate.pop_back();
        std::string attempt = candidate + ellipsis;
        if (MeasureText(attempt.c_str(), textSize) <= maxWidth) {
            return attempt;
        }
    }

    return ellipsis;
}

std::vector<std::string> buildWrappedLines(const std::string& text, int textSize, float maxWidth, int maxLines) {
    std::vector<std::string> words;
    std::stringstream stream(text);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }

    if (words.empty()) {
        return {""};
    }

    std::vector<std::string> lines;
    std::string current;

    for (size_t i = 0; i < words.size(); i++) {
        const std::string& nextWord = words[i];
        std::string attempt = current.empty() ? nextWord : (current + " " + nextWord);

        if (MeasureText(attempt.c_str(), textSize) <= maxWidth) {
            current = attempt;
            continue;
        }

        if (current.empty()) {
            current = trimToEllipsis(nextWord, textSize, maxWidth);
        }

        lines.push_back(current);
        current.clear();

        if (static_cast<int>(lines.size()) == maxLines - 1) {
            std::string remaining = nextWord;
            for (size_t j = i + 1; j < words.size(); j++) {
                remaining += " " + words[j];
            }
            lines.push_back(trimToEllipsis(remaining, textSize, maxWidth));
            return lines;
        }

        i--;
    }

    if (!current.empty()) {
        lines.push_back(current);
    }

    if (static_cast<int>(lines.size()) > maxLines) {
        lines.resize(maxLines);
        lines.back() = trimToEllipsis(lines.back(), textSize, maxWidth);
    }

    return lines;
}
}

Button::Button(float x, float y, float width, float height, const std::string& label)
    : label(label), hovered(false), clicked(false), textScale(1.0f),
      baseColor{70, 70, 150, 255}, hoverColor{100, 100, 200, 255},
      borderColor(Colors::LightGray), textColor(Colors::White) {
    bounds = {x, y, width, height};
}

void Button::update() {
    Vector2 pointerPos = Input::GetPointerPosition();
    hovered = CheckCollisionPointRec(pointerPos, bounds);
    clicked = hovered && Input::IsPointerPressed();
}

void Button::draw() {
    Color buttonColor = hovered ? hoverColor : baseColor;
    DrawRectangleRec(bounds, buttonColor);
    DrawRectangleLinesEx(bounds, 2, borderColor);

    float textSizeFloat = std::max(16.0f, std::min(42.0f, bounds.height * 0.24f * textScale));
    int textSize = static_cast<int>(textSizeFloat);
    int maxLines = bounds.height >= 70.0f ? 2 : 1;
    float horizontalPadding = std::max(8.0f, bounds.width * 0.08f);
    float maxTextWidth = std::max(1.0f, bounds.width - (2.0f * horizontalPadding));

    std::vector<std::string> lines = buildWrappedLines(label, textSize, maxTextWidth, maxLines);
    float lineSpacing = textSize * 1.15f;
    float totalHeight = static_cast<float>(lines.size()) * lineSpacing;
    float baseY = bounds.y + (bounds.height - totalHeight) * 0.5f;

    for (size_t i = 0; i < lines.size(); i++) {
        int textWidth = MeasureText(lines[i].c_str(), textSize);
        int textX = static_cast<int>(bounds.x + (bounds.width - textWidth) * 0.5f);
        int textY = static_cast<int>(baseY + i * lineSpacing);
        DrawText(lines[i].c_str(), textX, textY, textSize, textColor);
    }
}

bool Button::isClicked() const {
    return clicked;
}

void Button::setPosition(float x, float y) {
    bounds.x = x;
    bounds.y = y;
}

void Button::setSize(float width, float height) {
    bounds.width = width;
    bounds.height = height;
}

void Button::setTextScale(float scale) {
    textScale = std::max(0.5f, scale);
}

void Button::setColors(Color newBaseColor, Color newHoverColor, Color newBorderColor, Color newTextColor) {
    baseColor = newBaseColor;
    hoverColor = newHoverColor;
    borderColor = newBorderColor;
    textColor = newTextColor;
}

Rectangle Button::getBounds() const {
    return bounds;
}
