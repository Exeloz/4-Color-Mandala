#include "test_framework.h"
#include "../app/src/database/mandalaDatabase.h"

TEST_CASE(mandala_database_loads_default_mandalas) {
    MandalaDatabase database;
    const auto& all = database.getAllMandala();

    EXPECT_TRUE(all.size() >= 2);
}

TEST_CASE(mandala_database_can_lookup_known_ids) {
    MandalaDatabase database;

    std::shared_ptr<Mandala> tutorial = database.getMandalaById(1);
    EXPECT_NOT_NULL(tutorial);
    EXPECT_EQ(tutorial->getName(), std::string("Tutorial"));
    EXPECT_TRUE(tutorial->getRegionCount() > 0);

    std::shared_ptr<Mandala> real = database.getMandalaById(3);
    EXPECT_NOT_NULL(real);
    EXPECT_TRUE(real->getRegionCount() == 450);

    std::shared_ptr<Mandala> missing = database.getMandalaById(99999);
    EXPECT_NULL(missing);
}
