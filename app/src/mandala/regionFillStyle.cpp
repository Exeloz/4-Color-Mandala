#include "regionFillStyle.h"
#include "../rendering/fillPatternRenderer.h"

namespace {
class SolidRegionFillStyle final : public IRegionFillStyle {
public:
    void drawFill(const std::vector<Vector2>& vertices, Color fillColor) const override {
        FillPatternRenderer::drawPolygonFill(vertices, fillColor, FillPattern::Solid);
    }
};

class StripedRegionFillStyle final : public IRegionFillStyle {
public:
    void drawFill(const std::vector<Vector2>& vertices, Color fillColor) const override {
        FillPatternRenderer::drawPolygonFill(vertices, fillColor, FillPattern::Striped);
    }
};

class DottedRegionFillStyle final : public IRegionFillStyle {
public:
    void drawFill(const std::vector<Vector2>& vertices, Color fillColor) const override {
        FillPatternRenderer::drawPolygonFill(vertices, fillColor, FillPattern::Dotted);
    }
};
}

const IRegionFillStyle& RegionFillStyleFactory::getStyle(FillPattern pattern) {
    static SolidRegionFillStyle solidStyle;
    static StripedRegionFillStyle stripedStyle;
    static DottedRegionFillStyle dottedStyle;

    switch (pattern) {
        case FillPattern::Striped:
            return stripedStyle;
        case FillPattern::Dotted:
            return dottedStyle;
        case FillPattern::Solid:
        default:
            return solidStyle;
    }
}
