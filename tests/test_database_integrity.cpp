#include "test_framework.h"
#include "../app/src/database/mandalaDatabase.h"
#include <cmath>
#include <set>

namespace {
float signedArea(const std::vector<Vector2>& vertices) {
    if (vertices.size() < 3) {
        return 0.0f;
    }

    float areaTimesTwo = 0.0f;
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vector2& current = vertices[i];
        const Vector2& next = vertices[(i + 1) % vertices.size()];
        areaTimesTwo += current.x * next.y - next.x * current.y;
    }
    return areaTimesTwo * 0.5f;
}

bool nearEqual(const Vector2& a, const Vector2& b, float epsilon = 0.0001f) {
    return std::fabs(a.x - b.x) <= epsilon && std::fabs(a.y - b.y) <= epsilon;
}
}

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

TEST_CASE(database_generated_mandala_regions_are_clockwise_and_valid) {
    MandalaDatabase database;
    const auto& all = database.getAllMandala();

    for (const auto& mandala : all) {
        if (mandala->getId() == 1) {
            continue;
        }

        for (const auto& region : mandala->getRegions()) {
            const auto& vertices = region.getVertices();
            EXPECT_TRUE(vertices.size() >= 3);

            for (size_t i = 1; i < vertices.size(); ++i) {
                EXPECT_FALSE(nearEqual(vertices[i - 1], vertices[i]));
            }
            EXPECT_FALSE(nearEqual(vertices.front(), vertices.back()));

            const float area = signedArea(vertices);
            EXPECT_TRUE(std::fabs(area) > 0.0001f);
            EXPECT_TRUE(area < 0.0f);
        }
    }
}
