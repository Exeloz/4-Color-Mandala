#include "test_framework.h"
#include "test_helpers.h"

TEST_CASE(mandala_accessors_and_region_lookup) {
    Mandala mandala = makeTwoRegionMandala();

    EXPECT_EQ(mandala.getId(), 42);
    EXPECT_EQ(mandala.getName(), std::string("TestMandala"));
    EXPECT_EQ(mandala.getRegionCount(), 2);

    Region* region0 = mandala.getRegionById(0);
    EXPECT_NOT_NULL(region0);
    EXPECT_EQ(region0->getId(), 0);

    const Mandala& constRef = mandala;
    const Region* constRegion1 = constRef.getRegionById(1);
    EXPECT_NOT_NULL(constRegion1);
    EXPECT_EQ(constRegion1->getId(), 1);

    EXPECT_NULL(mandala.getRegionById(999));
}

TEST_CASE(mandala_get_region_at_point_respects_colorable_flag) {
    Mandala mandala = makeTwoRegionMandala();

    Region* found = mandala.getRegionAtPoint({5.0f, 5.0f});
    EXPECT_NOT_NULL(found);
    EXPECT_EQ(found->getId(), 0);

    Region* region0 = mandala.getRegionById(0);
    region0->setColorable(false);

    EXPECT_NULL(mandala.getRegionAtPoint({5.0f, 5.0f}));
}

TEST_CASE(mandala_fully_colored_and_valid_coloring) {
    Mandala mandala = makeTwoRegionMandala();

    EXPECT_FALSE(mandala.isFullyColored());
    EXPECT_FALSE(mandala.isValidColoring());

    mandala.getRegionById(0)->setColor(1);
    mandala.getRegionById(1)->setColor(2);

    EXPECT_TRUE(mandala.isFullyColored());
    EXPECT_TRUE(mandala.isValidColoring());

    mandala.getRegionById(1)->setColor(1);
    EXPECT_FALSE(mandala.isValidColoring());
}

TEST_CASE(mandala_non_colorable_uncolored_regions_do_not_block_validity) {
    Mandala mandala = makeTwoRegionMandala();

    mandala.getRegionById(0)->setColor(1);
    Region* region1 = mandala.getRegionById(1);
    region1->setColorable(false);

    EXPECT_FALSE(mandala.isFullyColored());
    EXPECT_TRUE(mandala.isValidColoring());
}

TEST_CASE(mandala_none_color_index_does_not_count_as_colored) {
    Mandala mandala = makeTwoRegionMandala();

    mandala.getRegionById(0)->setColor(1);
    mandala.getRegionById(1)->setColor(0);

    EXPECT_FALSE(mandala.isFullyColored());
    EXPECT_FALSE(mandala.isValidColoring());
}
