#pragma once
#include "region.h"
#include "adjacencyGraph.h"
#include <vector>
#include <string>

class Mandala {
public:
    Mandala(int id, const std::string& name, const std::vector<Region>& regions, 
            const AdjacencyGraph& adjacencyGraph,
            const std::string& regionsSourcePath = "",
            const std::string& adjacencySourcePath = "",
            bool hardMode = false);

    int getId() const;
    const std::string& getName() const;
    int getRegionCount() const;
    Region* getRegionAtPoint(Vector2 point);
    Region* getRegionById(int regionId);
    const Region* getRegionById(int regionId) const;
    const std::vector<Region>& getRegions() const;
    const AdjacencyGraph& getAdjacencyGraph() const;
    const std::string& getRegionsSourcePath() const;
    const std::string& getAdjacencySourcePath() const;
    bool isHardMode() const;
    bool isFullyColored() const;
    bool isValidColoring() const;
    void draw(const std::vector<Color>& colorPalette, bool ignoreColoring = false) const;

private:
    int id;
    std::string name;
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph;
    std::string regionsSourcePath;
    std::string adjacencySourcePath;
    bool hardMode;
};
