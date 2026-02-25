#pragma once

#include "../app/src/mandala/adjacencyGraph.h"
#include "../app/src/mandala/mandala.h"
#include "../app/src/ui/colors.h"
#include <vector>

inline bool colorsEqual(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline std::vector<Vector2> makeSquare(float minX, float minY, float size) {
    return {
        {minX, minY},
        {minX + size, minY},
        {minX + size, minY + size},
        {minX, minY + size}
    };
}

inline Mandala makeTwoRegionMandala() {
    std::vector<Region> regions;
    regions.emplace_back(0, makeSquare(0.0f, 0.0f, 10.0f));
    regions.emplace_back(1, makeSquare(10.0f, 0.0f, 10.0f));

    AdjacencyGraph graph(2);
    graph.addAdjacency(0, 1);

    return Mandala(42, "TestMandala", regions, graph);
}
