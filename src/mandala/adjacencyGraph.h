#pragma once
#include <vector>
#include <set>

class AdjacencyGraph {
public:
    AdjacencyGraph(int regionCount);

    void addAdjacency(int regionA, int regionB);
    bool areAdjacent(int regionA, int regionB) const;
    const std::set<int>& getAdjacentRegions(int regionId) const;

private:
    std::vector<std::set<int>> adjacencyList;
    static const std::set<int> emptySet;
};
