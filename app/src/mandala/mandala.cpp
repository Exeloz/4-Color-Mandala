#include "mandala.h"
#include <algorithm>

Mandala::Mandala(int id, const std::string& name, const std::vector<Region>& regions, 
                                 const AdjacencyGraph& adjacencyGraph,
                                 const std::string& regionsSourcePath,
                                 const std::string& adjacencySourcePath,
                                 bool hardMode,
                                 int minimumColorCount)
        : id(id),
            name(name),
            regions(regions),
            adjacencyGraph(adjacencyGraph),
            regionsSourcePath(regionsSourcePath),
            adjacencySourcePath(adjacencySourcePath),
            hardMode(hardMode),
            minimumColorCount(minimumColorCount),
            hasLastColorHint(false),
            lastColorHintTargetColor(-1),
            lastColorHintSolutionColorByRegionId(),
            lastColorHintInitialCountByRegionId() {}

int Mandala::getId() const {
    return id;
}

const std::string& Mandala::getName() const {
    return name;
}

int Mandala::getRegionCount() const {
    return regions.size();
}

Region* Mandala::getRegionAtPoint(Vector2 point) {
    for (auto it = regions.rbegin(); it != regions.rend(); ++it) {
        if (it->isPointInRegion(point) && it->isColorable()) {
            return &(*it);
        }
    }
    return nullptr;
}

Region* Mandala::getRegionById(int regionId) {
    for (auto& region : regions) {
        if (region.getId() == regionId) {
            return &region;
        }
    }
    return nullptr;
}

const Region* Mandala::getRegionById(int regionId) const {
    for (const auto& region : regions) {
        if (region.getId() == regionId) {
            return &region;
        }
    }
    return nullptr;
}

const std::vector<Region>& Mandala::getRegions() const {
    return regions;
}

const AdjacencyGraph& Mandala::getAdjacencyGraph() const {
    return adjacencyGraph;
}

const std::string& Mandala::getRegionsSourcePath() const {
    return regionsSourcePath;
}

const std::string& Mandala::getAdjacencySourcePath() const {
    return adjacencySourcePath;
}

bool Mandala::isHardMode() const {
    return hardMode;
}

int Mandala::getMinimumColorCount() const {
    return minimumColorCount;
}

bool Mandala::isFullyColored() const {
    return std::all_of(regions.begin(), regions.end(),
                      [](const Region& r) { return r.hasColor(); });
}

bool Mandala::isValidColoring() const {
    for (size_t i = 0; i < regions.size(); i++) {
        if (!regions[i].hasColor() && regions[i].isColorable()) {
            return false;
        }
        const auto& adjacentRegions = adjacencyGraph.getAdjacentRegions(i);
        for (int adjacentId : adjacentRegions) {
            if (regions[i].getColor() == regions[adjacentId].getColor()) {
                return false;
            }
        }
    }
    return true;
}

void Mandala::draw(const std::vector<Color>& colorPalette, bool ignoreColoring) const {
    for (const auto& region : regions) {
        region.draw(colorPalette, ignoreColoring);
    }
}

void Mandala::setLastColorHintData(int targetColor,
                                   const std::vector<int>& solutionColorByRegionId,
                                   const std::vector<int>& initialCountByRegionId) {
    if (targetColor <= 0 || solutionColorByRegionId.empty() || initialCountByRegionId.empty()
        || solutionColorByRegionId.size() != initialCountByRegionId.size()) {
        clearLastColorHintData();
        return;
    }

    hasLastColorHint = true;
    lastColorHintTargetColor = targetColor;
    lastColorHintSolutionColorByRegionId = solutionColorByRegionId;
    lastColorHintInitialCountByRegionId = initialCountByRegionId;
}

void Mandala::clearLastColorHintData() {
    hasLastColorHint = false;
    lastColorHintTargetColor = -1;
    lastColorHintSolutionColorByRegionId.clear();
    lastColorHintInitialCountByRegionId.clear();
}

bool Mandala::hasLastColorHintData() const {
    return hasLastColorHint
        && lastColorHintTargetColor > 0
        && !lastColorHintSolutionColorByRegionId.empty()
        && lastColorHintInitialCountByRegionId.size() == lastColorHintSolutionColorByRegionId.size();
}

int Mandala::getLastColorHintTargetColor() const {
    return hasLastColorHint ? lastColorHintTargetColor : -1;
}

int Mandala::getLastColorHintSolutionColor(int regionId) const {
    if (!hasLastColorHint || regionId < 0
        || static_cast<size_t>(regionId) >= lastColorHintSolutionColorByRegionId.size()) {
        return -1;
    }

    return lastColorHintSolutionColorByRegionId[static_cast<size_t>(regionId)];
}

int Mandala::getLastColorHintInitialCount(int regionId) const {
    if (!hasLastColorHint || regionId < 0
        || static_cast<size_t>(regionId) >= lastColorHintInitialCountByRegionId.size()) {
        return -1;
    }

    return lastColorHintInitialCountByRegionId[static_cast<size_t>(regionId)];
}

bool Mandala::isLastColorHintTrackedRegion(int regionId) const {
    return getLastColorHintInitialCount(regionId) >= 2;
}
