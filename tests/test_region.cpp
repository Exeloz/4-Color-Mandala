#include "test_framework.h"
#include "test_helpers.h"
#include "../app/src/mandala/region.h"

TEST_CASE(region_initial_state_and_color_updates) {
    Region region(7, makeSquare(0.0f, 0.0f, 10.0f));

    EXPECT_EQ(region.getId(), 7);
    EXPECT_FALSE(region.hasColor());
    EXPECT_EQ(region.getColor(), -1);
    EXPECT_TRUE(region.isColorable());
    EXPECT_TRUE(colorsEqual(region.getDefaultColor(), Colors::None));

    region.setColor(3);
    EXPECT_TRUE(region.hasColor());
    EXPECT_EQ(region.getColor(), 3);

    region.setColor(-1);
    EXPECT_FALSE(region.hasColor());
    EXPECT_EQ(region.getColor(), -1);
}

TEST_CASE(region_non_colorable_rejects_color_changes) {
    Region region(1, makeSquare(0.0f, 0.0f, 2.0f));
    region.setColorable(false);

    region.setColor(2);

    EXPECT_FALSE(region.hasColor());
    EXPECT_EQ(region.getColor(), -1);
}

TEST_CASE(region_centroid_handles_common_shapes) {
    Region triangle(1, {{0.0f, 0.0f}, {4.0f, 0.0f}, {0.0f, 4.0f}});
    Vector2 triCentroid = triangle.getCentroid();
    EXPECT_NEAR(triCentroid.x, 4.0f / 3.0f, 1e-4f);
    EXPECT_NEAR(triCentroid.y, 4.0f / 3.0f, 1e-4f);

    Region degenerate(2, {{0.0f, 0.0f}, {2.0f, 0.0f}, {4.0f, 0.0f}});
    Vector2 lineCentroid = degenerate.getCentroid();
    EXPECT_NEAR(lineCentroid.x, 2.0f, 1e-4f);
    EXPECT_NEAR(lineCentroid.y, 0.0f, 1e-4f);

    Region singlePoint(3, {{5.0f, -2.0f}});
    Vector2 pointCentroid = singlePoint.getCentroid();
    EXPECT_NEAR(pointCentroid.x, 5.0f, 1e-4f);
    EXPECT_NEAR(pointCentroid.y, -2.0f, 1e-4f);
}

TEST_CASE(region_point_in_polygon_detects_inside_and_outside) {
    Region region(1, makeSquare(0.0f, 0.0f, 10.0f));

    EXPECT_TRUE(region.isPointInRegion({5.0f, 5.0f}));
    EXPECT_FALSE(region.isPointInRegion({15.0f, 5.0f}));
    EXPECT_FALSE(region.isPointInRegion({-1.0f, -1.0f}));
}
