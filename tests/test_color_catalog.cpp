#include "test_framework.h"
#include "test_helpers.h"
#include "../app/src/ui/colorCatalog.h"

TEST_CASE(color_catalog_is_non_empty_and_contains_known_entries) {
    const std::vector<Color>& colors = ColorCatalog::getAvailableColors();

    EXPECT_TRUE(!colors.empty());

    bool foundBlack = false;
    bool foundRed = false;
    for (const Color& color : colors) {
        if (colorsEqual(color, Colors::Black)) {
            foundBlack = true;
        }
        if (colorsEqual(color, Colors::Red)) {
            foundRed = true;
        }
    }

    EXPECT_TRUE(foundBlack);
    EXPECT_TRUE(foundRed);
}

TEST_CASE(color_catalog_returns_same_static_collection) {
    const std::vector<Color>& first = ColorCatalog::getAvailableColors();
    const std::vector<Color>& second = ColorCatalog::getAvailableColors();

    EXPECT_EQ(&first, &second);
    EXPECT_EQ(first.size(), second.size());
}
