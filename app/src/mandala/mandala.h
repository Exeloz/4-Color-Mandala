#pragma once
#include "region.h"
#include "adjacencyGraph.h"
#include <vector>
#include <string>

class Mandala {
public:
    Mandala(int id, const std::string& name, const std::vector<Region>& regions, 
            const AdjacencyGraph& adjacencyGraph);

    int getId() const;
    const std::string& getName() const;
    int getRegionCount() const;
    Region* getRegionAtPoint(Vector2 point);
    Region* getRegionById(int regionId);
    const std::vector<Region>& getRegions() const;
    const AdjacencyGraph& getAdjacencyGraph() const;
    bool isFullyColored() const;
    bool isValidColoring() const;
    void draw(const std::vector<Color>& colorPalette, bool ignoreColoring = false) const;

private:
    int id;
    std::string name;
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph;
};
