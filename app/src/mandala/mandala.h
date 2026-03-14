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
            bool hardMode = false,
            int minimumColorCount = 0);

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
    int getMinimumColorCount() const;
    bool isFullyColored() const;
    bool isValidColoring() const;
    void draw(const std::vector<Color>& colorPalette, bool ignoreColoring = false) const;

    void setLastColorHintData(int targetColor,
                              const std::vector<int>& solutionColorByRegionId,
                              const std::vector<int>& initialCountByRegionId);
    void clearLastColorHintData();
    bool hasLastColorHintData() const;
    int getLastColorHintTargetColor() const;
    int getLastColorHintSolutionColor(int regionId) const;
    int getLastColorHintInitialCount(int regionId) const;
    bool isLastColorHintTrackedRegion(int regionId) const;

private:
    int id;
    std::string name;
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph;
    std::string regionsSourcePath;
    std::string adjacencySourcePath;
    bool hardMode;
    int minimumColorCount;
    bool hasLastColorHint;
    int lastColorHintTargetColor;
    std::vector<int> lastColorHintSolutionColorByRegionId;
    std::vector<int> lastColorHintInitialCountByRegionId;
};
