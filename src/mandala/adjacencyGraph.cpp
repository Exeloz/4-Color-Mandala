#include "adjacencyGraph.h"

const std::set<int> AdjacencyGraph::emptySet;

AdjacencyGraph::AdjacencyGraph(int regionCount) {
    adjacencyList.resize(regionCount);
}

void AdjacencyGraph::addAdjacency(int regionA, int regionB) {
    if (regionA >= 0 && regionA < static_cast<int>(adjacencyList.size()) &&
        regionB >= 0 && regionB < static_cast<int>(adjacencyList.size()) &&
        regionA != regionB) {
        adjacencyList[regionA].insert(regionB);
        adjacencyList[regionB].insert(regionA);
    }
}

bool AdjacencyGraph::areAdjacent(int regionA, int regionB) const {
    if (regionA >= 0 && regionA < static_cast<int>(adjacencyList.size())) {
        return adjacencyList[regionA].count(regionB) > 0;
    }
    return false;
}

const std::set<int>& AdjacencyGraph::getAdjacentRegions(int regionId) const {
    if (regionId >= 0 && regionId < static_cast<int>(adjacencyList.size())) {
        return adjacencyList[regionId];
    }
    return emptySet;
}
