#pragma once
#include <raylib.h>
#include <vector>

class Region {
public:
    Region(int id, const std::vector<Vector2>& vertices);

    int getId() const;
    void setColor(int colorIndex);
    int getColor() const;
    bool hasColor() const;
    void setDefaultColor(Color color);
    Color getDefaultColor() const;
    void setColorable(bool canColor);
    bool isColorable() const;
    const std::vector<Vector2>& getVertices() const;
    bool isPointInRegion(Vector2 point) const;
    void draw(const std::vector<Color>& colorPalette) const;

private:
    int id;
    std::vector<Vector2> vertices;
    int colorIndex;
    bool colored;
    Color defaultColor;
    bool colorable;
};
