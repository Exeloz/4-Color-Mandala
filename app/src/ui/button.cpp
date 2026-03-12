#include "button.h"
#include "colors.h"
#include "input.h"
#include <algorithm>
#include "raymath.h"
#include <sstream>
#include <vector>

namespace {
Color blendColor(const Color& from, const Color& to, float t) {
    const float clamped = Clamp(t, 0.0f, 1.0f);
    return {
        static_cast<unsigned char>(from.r + (to.r - from.r) * clamped),
        static_cast<unsigned char>(from.g + (to.g - from.g) * clamped),
        static_cast<unsigned char>(from.b + (to.b - from.b) * clamped),
        static_cast<unsigned char>(from.a + (to.a - from.a) * clamped)
    };
}

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
    : label(label), hovered(false), clicked(false), enabled(true), textScale(1.0f), hoverBlend(0.0f), pressBlend(0.0f),
      baseColor{70, 70, 150, 255}, hoverColor{100, 100, 200, 255},
      borderColor(Colors::LightGray), textColor(Colors::White) {
    bounds = {x, y, width, height};
}

void Button::update() {
    if (!enabled) {
        hovered = false;
        clicked = false;
        hoverBlend = 0.0f;
        pressBlend = 0.0f;
        return;
    }
    Vector2 pointerPos = Input::GetPointerPosition();
    hovered = CheckCollisionPointRec(pointerPos, bounds);
    clicked = hovered && Input::IsPointerPressed();

    const float deltaTime = GetFrameTime();
    const float hoverTarget = hovered ? 1.0f : 0.0f;
    const float pressTarget = (hovered && Input::IsPointerDown()) ? 1.0f : 0.0f;
    const float hoverLerp = std::min(1.0f, deltaTime * 12.0f);
    const float pressLerp = std::min(1.0f, deltaTime * 18.0f);
    hoverBlend = Lerp(hoverBlend, hoverTarget, hoverLerp);
    pressBlend = Lerp(pressBlend, pressTarget, pressLerp);
}

void Button::draw() {
    const Color drawBase = enabled ? baseColor : Color{110, 110, 110, 70};
    const Color buttonColor = enabled ? blendColor(baseColor, hoverColor, hoverBlend) : drawBase;

    Rectangle animatedBounds = bounds;
    animatedBounds.y -= (2.0f * hoverBlend);
    animatedBounds.y += (1.5f * pressBlend);

    Rectangle shadowBounds = animatedBounds;
    shadowBounds.y += 3.0f;

    DrawRectangleRounded(shadowBounds, 0.18f, 8, Fade(Colors::Black, 0.10f + 0.05f * hoverBlend));
    DrawRectangleRounded(animatedBounds, 0.18f, 8, buttonColor);
    DrawRectangleRoundedLinesEx(animatedBounds, 0.18f, 8, 2.0f, borderColor);

    float textSizeFloat = std::max(16.0f, std::min(42.0f, animatedBounds.height * 0.24f * textScale));
    textSizeFloat *= (1.0f + hoverBlend * 0.04f - pressBlend * 0.04f);
    int textSize = static_cast<int>(textSizeFloat);
    int maxLines = animatedBounds.height >= 70.0f ? 2 : 1;
    float horizontalPadding = std::max(8.0f, animatedBounds.width * 0.08f);
    float maxTextWidth = std::max(1.0f, animatedBounds.width - (2.0f * horizontalPadding));

    std::vector<std::string> lines = buildWrappedLines(label, textSize, maxTextWidth, maxLines);
    float lineSpacing = textSize * 1.15f;
    float totalHeight = static_cast<float>(lines.size()) * lineSpacing;
    float baseY = animatedBounds.y + (animatedBounds.height - totalHeight) * 0.5f;

    for (size_t i = 0; i < lines.size(); i++) {
        int textWidth = MeasureText(lines[i].c_str(), textSize);
        int textX = static_cast<int>(animatedBounds.x + (animatedBounds.width - textWidth) * 0.5f);
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

void Button::setLabel(const std::string& newLabel) {
    label = newLabel;
}

void Button::setTextScale(float scale) {
    textScale = std::max(0.5f, scale);
}

void Button::setEnabled(bool value) {
    enabled = value;
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
