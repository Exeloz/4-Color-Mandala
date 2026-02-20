#pragma once

#include "../rendering/fillPattern.h"
#include <raylib.h>
#include <vector>

class IRegionFillStyle {
public:
    virtual ~IRegionFillStyle() = default;
    virtual void drawFill(const std::vector<Vector2>& vertices, Color fillColor,
                          FillPattern style) const = 0;
};

class RegionFillStyleFactory {
public:
    static const IRegionFillStyle& getStyle(FillPatternType patternType);
};
