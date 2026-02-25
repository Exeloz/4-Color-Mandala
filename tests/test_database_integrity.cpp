#include "test_framework.h"
#include "../app/src/database/mandalaDatabase.h"
#include <set>

TEST_CASE(database_regions_have_unique_ids) {
    MandalaDatabase database;
    const auto& all = database.getAllMandala();

    for (const auto& mandala : all) {
        const auto& regions = mandala->getRegions();
        std::set<int> ids;

        for (const auto& region : regions) {
            ids.insert(region.getId());
        }

        EXPECT_EQ(ids.size(), regions.size());
    }
}

TEST_CASE(database_region_ids_can_be_resolved_by_lookup) {
    MandalaDatabase database;
    const auto& all = database.getAllMandala();

    for (const auto& mandala : all) {
        const auto& regions = mandala->getRegions();
        for (const auto& region : regions) {
            const Region* found = mandala->getRegionById(region.getId());
            EXPECT_NOT_NULL(found);
            EXPECT_EQ(found->getId(), region.getId());
        }
    }
}

TEST_CASE(database_adjacency_is_symmetric_and_in_bounds) {
    MandalaDatabase database;
    const auto& all = database.getAllMandala();

    for (const auto& mandala : all) {
        const auto& graph = mandala->getAdjacencyGraph();
        const int regionCount = mandala->getRegionCount();

        for (int regionId = 0; regionId < regionCount; ++regionId) {
            const auto& neighbors = graph.getAdjacentRegions(regionId);
            for (int neighborId : neighbors) {
                EXPECT_TRUE(neighborId >= 0);
                EXPECT_TRUE(neighborId < regionCount);
                EXPECT_TRUE(graph.areAdjacent(neighborId, regionId));
            }
        }
    }
}

TEST_CASE(database_real_mandala_contains_edges) {
    MandalaDatabase database;
    auto real = database.getMandalaById(3);
    EXPECT_NOT_NULL(real);

    const auto& graph = real->getAdjacencyGraph();
    bool foundAnyNeighbor = false;

    for (int regionId = 0; regionId < real->getRegionCount(); ++regionId) {
        if (!graph.getAdjacentRegions(regionId).empty()) {
            foundAnyNeighbor = true;
            break;
        }
    }

    EXPECT_TRUE(foundAnyNeighbor);
}
