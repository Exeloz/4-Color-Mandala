#include "colorWheelPicker.h"
#include "input.h"
#include "rlgl.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
        constexpr float HUE_SEGMENT_DEGREES = 3.0f;
}

ColorWheelPicker::ColorWheelPicker()
    : bounds{0, 0, 0, 0},
      center{0, 0},
            radius(0.0f),
            valueSliderRect{0, 0, 0, 0},
      hue(200.0f),
      saturation(0.75f),
      value(0.9f),
            draggingWheel(false),
            draggingValue(false) {
}

void ColorWheelPicker::setBounds(const Rectangle& newBounds) {
    bounds = newBounds;
    updateGeometry();
}

void ColorWheelPicker::setColor(const Color& color) {
    Vector3 hsv = ColorToHSV(color);
    hue = hsv.x;
    if (hue < 0.0f) {
        hue = 0.0f;
    }
    saturation = Clamp(hsv.y, 0.0f, 1.0f);
    value = Clamp(hsv.z, 0.0f, 1.0f);
}

void ColorWheelPicker::update() {
    if (!Input::IsPointerDown()) {
        draggingWheel = false;
        draggingValue = false;
        return;
    }

    Vector2 pointer = Input::GetPointerPosition();

    if (Input::IsPointerPressed() && !draggingWheel && !draggingValue) {
        if (isPointerInsideWheel(pointer)) {
            draggingWheel = true;
        } else if (isPointerInsideValueSlider(pointer)) {
            draggingValue = true;
        } else {
            return;
        }
    }

    if (draggingWheel) {
        updateWheelFromPointer(pointer);
    }

    if (draggingValue) {
        updateValueFromPointer(pointer);
    }
}

void ColorWheelPicker::draw(Color panelBackground) const {
    (void)panelBackground;

    rlBegin(RL_TRIANGLES);
    for (float startAngle = 0.0f; startAngle < 360.0f; startAngle += HUE_SEGMENT_DEGREES) {
        float endAngle = std::min(360.0f, startAngle + HUE_SEGMENT_DEGREES);

        float startRadians = startAngle * DEG2RAD;
        float endRadians = endAngle * DEG2RAD;

        Vector2 p1 = {center.x + std::sin(startRadians) * radius, center.y - std::cos(startRadians) * radius};
        Vector2 p2 = {center.x + std::sin(endRadians) * radius, center.y - std::cos(endRadians) * radius};

        Color c1 = ColorFromHSV(startAngle, 1.0f, 1.0f);
        Color c2 = ColorFromHSV(endAngle, 1.0f, 1.0f);

        rlColor4ub(c1.r, c1.g, c1.b, c1.a);
        rlVertex2f(p1.x, p1.y);

        rlColor4ub(WHITE.r, WHITE.g, WHITE.b, WHITE.a);
        rlVertex2f(center.x, center.y);

        rlColor4ub(c2.r, c2.g, c2.b, c2.a);
        rlVertex2f(p2.x, p2.y);
    }
    rlEnd();

    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, Fade(BLACK, 0.55f));

    float hueRadians = hue * DEG2RAD;
    Vector2 wheelHandle = {
        center.x + std::sin(hueRadians) * (saturation * radius),
        center.y - std::cos(hueRadians) * (saturation * radius)
    };

    Color selected = ColorFromHSV(hue, saturation, value);
    float luminance = (0.299f * selected.r + 0.587f * selected.g + 0.114f * selected.b) / 255.0f;
    Color handleOutline = (luminance < 0.5f) ? WHITE : BLACK;

    DrawCircleV(wheelHandle, 7.0f, Fade(WHITE, 0.35f));
    DrawCircleLines(static_cast<int>(wheelHandle.x), static_cast<int>(wheelHandle.y), 7.0f, handleOutline);

    DrawRectangleGradientEx(valueSliderRect, WHITE, WHITE, BLACK, BLACK);
    DrawRectangleLinesEx(valueSliderRect, 2.0f, DARKGRAY);

    float sliderHandleY = valueSliderRect.y + (1.0f - value) * valueSliderRect.height;
    Rectangle sliderHandle = {
        valueSliderRect.x - 3.0f,
        sliderHandleY - 4.0f,
        valueSliderRect.width + 6.0f,
        8.0f
    };
    DrawRectangleRec(sliderHandle, WHITE);
    DrawRectangleLinesEx(sliderHandle, 1.5f, BLACK);
}

Color ColorWheelPicker::getSelectedColor() const {
    return ColorFromHSV(hue, saturation, value);
}

void ColorWheelPicker::updateGeometry() {
    float sliderWidth = std::max(14.0f, bounds.width * 0.08f);
    float sliderVerticalPadding = std::max(12.0f, bounds.height * 0.08f);
    float sliderGap = std::max(10.0f, bounds.width * 0.05f);

    valueSliderRect = {
        bounds.x + bounds.width - sliderWidth,
        bounds.y + sliderVerticalPadding,
        sliderWidth,
        std::max(24.0f, bounds.height - (2.0f * sliderVerticalPadding))
    };

    float wheelWidth = std::max(20.0f, valueSliderRect.x - bounds.x - sliderGap);
    float wheelHeight = std::max(20.0f, bounds.height);
    float diameter = std::min(wheelWidth, wheelHeight) * 0.92f;
    radius = std::max(12.0f, diameter * 0.5f);

    center = {
        bounds.x + (wheelWidth * 0.5f),
        bounds.y + (bounds.height * 0.5f)
    };
}

bool ColorWheelPicker::isPointerInsideWheel(Vector2 pointer) const {
    return Vector2Distance(pointer, center) <= (radius + 10.0f);
}

bool ColorWheelPicker::isPointerInsideValueSlider(Vector2 pointer) const {
    Rectangle touchRect = {
        valueSliderRect.x - 8.0f,
        valueSliderRect.y,
        valueSliderRect.width + 16.0f,
        valueSliderRect.height
    };
    return CheckCollisionPointRec(pointer, touchRect);
}

void ColorWheelPicker::updateWheelFromPointer(Vector2 pointer) {
    Vector2 direction = Vector2Subtract(pointer, center);
    float distance = Vector2Length(direction);
    if (distance > radius && distance > 0.0f) {
        direction = Vector2Scale(direction, radius / distance);
        distance = radius;
    }

    saturation = Clamp(distance / std::max(1.0f, radius), 0.0f, 1.0f);

    float angle = std::atan2(direction.x, -direction.y);
    if (angle < 0.0f) {
        angle += (2.0f * PI);
    }
    hue = angle * RAD2DEG;
}

void ColorWheelPicker::updateValueFromPointer(Vector2 pointer) {
    float clampedY = Clamp(pointer.y, valueSliderRect.y, valueSliderRect.y + valueSliderRect.height);
    value = 1.0f - ((clampedY - valueSliderRect.y) / valueSliderRect.height);
    value = Clamp(value, 0.0f, 1.0f);
}
