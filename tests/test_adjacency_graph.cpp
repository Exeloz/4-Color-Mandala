#include "test_framework.h"
#include "../app/src/mandala/adjacencyGraph.h"

TEST_CASE(adjacency_graph_adds_bidirectional_edges) {
    AdjacencyGraph graph(4);
    graph.addAdjacency(1, 2);

    EXPECT_TRUE(graph.areAdjacent(1, 2));
    EXPECT_TRUE(graph.areAdjacent(2, 1));
    EXPECT_EQ(graph.getAdjacentRegions(1).size(), 1u);
    EXPECT_EQ(graph.getAdjacentRegions(2).size(), 1u);
}

TEST_CASE(adjacency_graph_rejects_invalid_and_self_edges) {
    AdjacencyGraph graph(3);
    graph.addAdjacency(0, 0);
    graph.addAdjacency(-1, 1);
    graph.addAdjacency(0, 5);

    EXPECT_FALSE(graph.areAdjacent(0, 0));
    EXPECT_FALSE(graph.areAdjacent(0, 1));
    EXPECT_TRUE(graph.getAdjacentRegions(0).empty());
}

TEST_CASE(adjacency_graph_invalid_queries_are_safe) {
    AdjacencyGraph graph(2);
    graph.addAdjacency(0, 1);

    EXPECT_FALSE(graph.areAdjacent(-1, 1));
    EXPECT_FALSE(graph.areAdjacent(4, 1));
    EXPECT_TRUE(graph.getAdjacentRegions(-1).empty());
    EXPECT_TRUE(graph.getAdjacentRegions(99).empty());
}
