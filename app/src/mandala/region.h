#pragma once
#include <raylib.h>
#include <vector>
#include "regionFillStyle.h"

class Region {
public:
    Region(int id, const std::vector<Vector2>& vertices);

    int getId() const;
    void setColor(int colorIndex);
    int getColor() const;
    bool hasColor() const;
    void setDefaultColor(Color color);
    Color getDefaultColor() const;
    void setFillPattern(FillPattern pattern);
    FillPattern getFillPattern() const;
    void setColorable(bool canColor);
    bool isColorable() const;
    const std::vector<Vector2>& getVertices() const;
    Vector2 getCentroid() const;
    Vector2 getInteriorPoint() const;
    bool isPointInRegion(Vector2 point) const;
    void draw(const std::vector<Color>& colorPalette, bool ignoreColoring = false) const;
    void drawWithColor(Color fillColor, Color borderColor, float borderWidth,
                       FillPattern pattern = FillPattern()) const;

private:
    void buildTriangleCache();
    void drawCachedSolidFill(Color fillColor) const;

    int id;
    std::vector<Vector2> vertices;
    std::vector<Vector2> triangleVertices;
    int colorIndex;
    bool colored;
    Color defaultColor;
    FillPattern fillPattern;
    bool colorable;
};
