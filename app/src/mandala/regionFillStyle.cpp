#include "regionFillStyle.h"
#include "../rendering/fillPatternRenderer.h"

namespace {
class SolidRegionFillStyle final : public IRegionFillStyle {
public:
    void drawFill(const std::vector<Vector2>& vertices, Color fillColor,
                  FillPattern style) const override {
        FillPatternRenderer::drawPolygonFill(vertices, fillColor, style);
    }
};

class StripedRegionFillStyle final : public IRegionFillStyle {
public:
    void drawFill(const std::vector<Vector2>& vertices, Color fillColor,
                  FillPattern style) const override {
        FillPatternRenderer::drawPolygonFill(vertices, fillColor, style);
    }
};

class DottedRegionFillStyle final : public IRegionFillStyle {
public:
    void drawFill(const std::vector<Vector2>& vertices, Color fillColor,
                  FillPattern style) const override {
        FillPatternRenderer::drawPolygonFill(vertices, fillColor, style);
    }
};
}

const IRegionFillStyle& RegionFillStyleFactory::getStyle(FillPatternType patternType) {
    static SolidRegionFillStyle solidStyle;
    static StripedRegionFillStyle stripedStyle;
    static DottedRegionFillStyle dottedStyle;

    switch (patternType) {
        case FillPatternType::Striped:
            return stripedStyle;
        case FillPatternType::Dotted:
            return dottedStyle;
        case FillPatternType::Solid:
        default:
            return solidStyle;
    }
}
