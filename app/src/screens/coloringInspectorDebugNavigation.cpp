#include "coloringInspector.h"

#include <algorithm>

std::vector<int> ColoringInspector::collectSortedRegionIds(const std::vector<Region>& regions) {
    std::vector<int> ids;
    ids.reserve(regions.size());
    for (const auto& region : regions) {
        if (region.isColorable()) {
            ids.push_back(region.getId());
        }
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

int ColoringInspector::cycleRegionId(const std::vector<int>& sortedIds, int currentId, int direction) {
    if (sortedIds.empty()) {
        return -1;
    }

    if (currentId < 0) {
        return sortedIds.front();
    }

    auto it = std::lower_bound(sortedIds.begin(), sortedIds.end(), currentId);
    if (it == sortedIds.end() || *it != currentId) {
        if (direction > 0) {
            return (it == sortedIds.end()) ? sortedIds.front() : *it;
        }

        if (it == sortedIds.begin()) {
            return sortedIds.back();
        }

        --it;
        return *it;
    }

    int index = static_cast<int>(it - sortedIds.begin());
    int size = static_cast<int>(sortedIds.size());
    int nextIndex = (index + direction + size) % size;
    return sortedIds[nextIndex];
}

int ColoringInspector::getRegionIdAtWorldPosition(const Mandala& mandala, Vector2 worldPos) const {
    const auto& regions = mandala.getRegions();
    for (auto it = regions.rbegin(); it != regions.rend(); ++it) {
        if (it->isPointInRegion(worldPos) && it->isColorable()) {
            return it->getId();
        }
    }

    return -1;
}

void ColoringInspector::centerCameraOnRegion(const Mandala& mandala, int regionId, Camera2D& camera) const {
    const Region* region = mandala.getRegionById(regionId);
    if (region == nullptr) {
        return;
    }

    camera.target = region->getCentroid();
}
