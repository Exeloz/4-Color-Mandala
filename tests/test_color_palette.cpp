#include "test_framework.h"
#include "test_helpers.h"
#include "../app/src/ui/colorPalette.h"

TEST_CASE(color_palette_defaults_are_stable) {
    ColorPalette palette;

    EXPECT_EQ(palette.getColorCount(), 5);
    EXPECT_EQ(palette.getSelectedColorIndex(), 0);
    EXPECT_TRUE(colorsEqual(palette.getColor(0), Colors::None));
    EXPECT_TRUE(colorsEqual(palette.getColor(1), Colors::Red));
}

TEST_CASE(color_palette_validates_indexes) {
    ColorPalette palette;

    EXPECT_TRUE(palette.isValidColorIndex(0));
    EXPECT_FALSE(palette.isValidColorIndex(-1));
    EXPECT_FALSE(palette.isValidColorIndex(99));

    Color fallback = palette.getColor(-2);
    EXPECT_TRUE(colorsEqual(fallback, Color{0, 0, 0, 255}));
}

TEST_CASE(color_palette_selected_index_updates_only_when_valid) {
    ColorPalette palette;
    palette.setSelectedColorIndex(2);
    EXPECT_EQ(palette.getSelectedColorIndex(), 2);

    palette.setSelectedColorIndex(99);
    EXPECT_EQ(palette.getSelectedColorIndex(), 2);
}

TEST_CASE(color_palette_set_colors_replaces_and_rebounds_selection) {
    ColorPalette palette;
    palette.setSelectedColorIndex(4);

    std::vector<Color> custom = {Colors::Black, Colors::White};
    palette.setColors(custom);

    EXPECT_EQ(palette.getColorCount(), 2);
    EXPECT_EQ(palette.getSelectedColorIndex(), 0);
    EXPECT_TRUE(colorsEqual(palette.getColor(1), Colors::White));

    std::vector<Color> empty;
    palette.setColors(empty);
    EXPECT_EQ(palette.getColorCount(), 2);
}
