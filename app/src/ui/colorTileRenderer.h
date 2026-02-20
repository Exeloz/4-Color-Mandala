#ifndef COLOR_TILE_RENDERER_H
#define COLOR_TILE_RENDERER_H

#include "raylib.h"
#include "../rendering/fillPattern.h"

class ColorTileRenderer {
public:
    static bool isNoneColor(const Color& color);
    static void drawColorTile(const Color& color, Rectangle bounds, float uiScale,
                              FillPattern style = FillPattern());
};

#endif
