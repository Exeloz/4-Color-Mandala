#pragma once
#include <raylib.h>
#include <string>

class Button {
public:
    Button(float x, float y, float width, float height, const std::string& label);

    void update();
    void draw();
    bool isClicked() const;

    void setPosition(float x, float y);
    void setSize(float width, float height);
    Rectangle getBounds() const;

private:
    Rectangle bounds;
    std::string label;
    bool hovered;
    bool clicked;
};
