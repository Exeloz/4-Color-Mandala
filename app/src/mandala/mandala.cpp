#include "mandala.h"
#include <algorithm>

Mandala::Mandala(int id, const std::string& name, const std::vector<Region>& regions, 
                                 const AdjacencyGraph& adjacencyGraph,
                                 const std::string& regionsSourcePath,
                                 const std::string& adjacencySourcePath)
        : id(id),
            name(name),
            regions(regions),
            adjacencyGraph(adjacencyGraph),
            regionsSourcePath(regionsSourcePath),
            adjacencySourcePath(adjacencySourcePath) {}

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
