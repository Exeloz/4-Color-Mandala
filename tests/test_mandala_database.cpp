#include "test_framework.h"
#include "../app/src/database/mandalaDatabase.h"
#include "../app/src/ui/colors.h"

TEST_CASE(mandala_database_loads_default_mandalas) {
    MandalaDatabase database;
    const auto& listItems = database.getMandalaListItems();
    const auto& loaded = database.getAllMandala();

    EXPECT_TRUE(listItems.size() >= 2);
    EXPECT_TRUE(loaded.size() >= 1);
}

TEST_CASE(mandala_database_can_lookup_known_ids) {
    MandalaDatabase database;

    std::shared_ptr<Mandala> tutorial = database.getMandalaById(0);
    EXPECT_NOT_NULL(tutorial);
    EXPECT_EQ(tutorial->getName(), std::string("Tutorial"));
    EXPECT_TRUE(tutorial->getRegionCount() > 0);

    database.loadMandala(1);
    std::shared_ptr<Mandala> real = database.getMandalaById(1);
    EXPECT_NOT_NULL(real);
    EXPECT_TRUE(real->getRegionCount() == 450);

    std::shared_ptr<Mandala> missing = database.getMandalaById(99999);
    EXPECT_NULL(missing);
}

TEST_CASE(mandala_database_loads_region_colorability_and_default_color_from_json) {
    MandalaDatabase database;
    database.loadMandala(1);
    std::shared_ptr<Mandala> real = database.getMandalaById(1);
    EXPECT_NOT_NULL(real);

    Region* region = real->getRegionById(0);
    EXPECT_NOT_NULL(region);
    EXPECT_FALSE(region->isColorable());

    const Color color = region->getDefaultColor();
    EXPECT_EQ(color.r, Colors::Black.r);
    EXPECT_EQ(color.g, Colors::Black.g);
    EXPECT_EQ(color.b, Colors::Black.b);
    EXPECT_EQ(color.a, Colors::Black.a);
}
