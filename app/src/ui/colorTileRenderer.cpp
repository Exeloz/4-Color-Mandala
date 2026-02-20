#include "colorTileRenderer.h"
#include "colors.h"
#include "../rendering/fillPatternRenderer.h"
#include <algorithm>

bool ColorTileRenderer::isNoneColor(const Color& color) {
    return color.r == Colors::None.r &&
           color.g == Colors::None.g &&
           color.b == Colors::None.b &&
           color.a == Colors::None.a;
}

void ColorTileRenderer::drawColorTile(const Color& color, Rectangle bounds, float uiScale,
                                      FillPattern style) {
    if (!isNoneColor(color)) {
        FillPatternRenderer::drawRectangleFill(bounds, color, style);
        return;
    }

    DrawRectangleRec(bounds, Colors::White);

    float pad = std::max(2.0f, 3.0f * uiScale);
    Vector2 a = {bounds.x + pad, bounds.y + pad};
    Vector2 b = {bounds.x + bounds.width - pad, bounds.y + bounds.height - pad};
    Vector2 c = {bounds.x + pad, bounds.y + bounds.height - pad};
    Vector2 d = {bounds.x + bounds.width - pad, bounds.y + pad};
    float stroke = std::max(2.0f, 3.0f * uiScale);
    DrawLineEx(a, b, stroke, Colors::Crimson);
    DrawLineEx(c, d, stroke, Fade(Colors::Crimson, 0.55f));
}
