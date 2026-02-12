#include "mandala.h"
#include <algorithm>

Mandala::Mandala(int id, const std::string& name, const std::vector<Region>& regions, 
                 const AdjacencyGraph& adjacencyGraph)
    : id(id), name(name), regions(regions), adjacencyGraph(adjacencyGraph) {}

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
    for (auto& region : regions) {
        if (region.isPointInRegion(point)) {
            return &region;
        }
    }
    return nullptr;
}

const std::vector<Region>& Mandala::getRegions() const {
    return regions;
}

bool Mandala::isFullyColored() const {
    return std::all_of(regions.begin(), regions.end(),
                      [](const Region& r) { return r.hasColor(); });
}

bool Mandala::isValidColoring() const {
    for (size_t i = 0; i < regions.size(); i++) {
        if (!regions[i].hasColor()) {
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

void Mandala::draw(const std::vector<Color>& colorPalette) const {
    for (const auto& region : regions) {
        region.draw(colorPalette);
    }
}
