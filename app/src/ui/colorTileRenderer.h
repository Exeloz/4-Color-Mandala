#ifndef COLOR_TILE_RENDERER_H
#define COLOR_TILE_RENDERER_H

#include "raylib.h"

class ColorTileRenderer {
public:
    static bool isNoneColor(const Color& color);
    static void drawColorTile(const Color& color, Rectangle bounds, float uiScale);
};

#endif
