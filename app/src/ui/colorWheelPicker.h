#pragma once

#include "raylib.h"

class ColorWheelPicker {
public:
    ColorWheelPicker();

    void setBounds(const Rectangle& bounds);
    void setColor(const Color& color);

    void update();
    void draw(Color panelBackground) const;

    Color getSelectedColor() const;

private:
    Rectangle bounds;
    Vector2 center;
    float radius;
    Rectangle valueSliderRect;

    float hue;
    float saturation;
    float value;

    bool draggingWheel;
    bool draggingValue;

    void updateGeometry();
    bool isPointerInsideWheel(Vector2 pointer) const;
    bool isPointerInsideValueSlider(Vector2 pointer) const;
    void updateWheelFromPointer(Vector2 pointer);
    void updateValueFromPointer(Vector2 pointer);
};
