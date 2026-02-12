#include "button.h"
#include "colors.h"

Button::Button(float x, float y, float width, float height, const std::string& label)
    : label(label), hovered(false), clicked(false) {
    bounds = {x, y, width, height};
}

void Button::update() {
    Vector2 mousePos = GetMousePosition();
    hovered = CheckCollisionPointRec(mousePos, bounds);
    clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Button::draw() {
    Color buttonColor = hovered ? Color{100, 100, 200, 255} : Color{70, 70, 150, 255};
    DrawRectangleRec(bounds, buttonColor);
    DrawRectangleLinesEx(bounds, 2, Colors::LightGray);

    int textWidth = MeasureText(label.c_str(), 20);
    int textX = bounds.x + (bounds.width - textWidth) / 2;
    int textY = bounds.y + (bounds.height - 20) / 2;
    DrawText(label.c_str(), textX, textY, 20, Colors::White);
}

bool Button::isClicked() const {
    return clicked;
}

void Button::setPosition(float x, float y) {
    bounds.x = x;
    bounds.y = y;
}

Rectangle Button::getBounds() const {
    return bounds;
}
