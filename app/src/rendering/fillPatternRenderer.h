#pragma once

#include "fillPattern.h"
#include <raylib.h>
#include <vector>

class FillPatternRenderer {
public:
    static void drawPolygonFill(const std::vector<Vector2>& vertices, Color fillColor, FillPattern pattern);
    static void drawRectangleFill(Rectangle bounds, Color fillColor, FillPattern pattern);
};
