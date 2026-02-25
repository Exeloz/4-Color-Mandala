#include "test_framework.h"
#include "../app/src/rendering/fillPattern.h"
#include "../app/src/mandala/regionFillStyle.h"

TEST_CASE(fill_pattern_defaults_are_consistent) {
    FillPattern pattern;

    EXPECT_EQ(static_cast<int>(pattern.type), static_cast<int>(FillPatternType::Solid));
    EXPECT_EQ(pattern.size, 1.0f);
    EXPECT_FALSE(pattern.useAccentColor);
    EXPECT_EQ(pattern.accentColor.r, BLANK.r);
    EXPECT_EQ(pattern.accentColor.g, BLANK.g);
    EXPECT_EQ(pattern.accentColor.b, BLANK.b);
    EXPECT_EQ(pattern.accentColor.a, BLANK.a);
}

TEST_CASE(region_fill_style_factory_returns_stable_references) {
    const IRegionFillStyle& solidA = RegionFillStyleFactory::getStyle(FillPatternType::Solid);
    const IRegionFillStyle& solidB = RegionFillStyleFactory::getStyle(FillPatternType::Solid);

    EXPECT_EQ(&solidA, &solidB);
}

TEST_CASE(region_fill_style_factory_distinguishes_pattern_types) {
    const IRegionFillStyle& solid = RegionFillStyleFactory::getStyle(FillPatternType::Solid);
    const IRegionFillStyle& striped = RegionFillStyleFactory::getStyle(FillPatternType::Striped);
    const IRegionFillStyle& dotted = RegionFillStyleFactory::getStyle(FillPatternType::Dotted);

    EXPECT_NE(&solid, &striped);
    EXPECT_NE(&solid, &dotted);
}
