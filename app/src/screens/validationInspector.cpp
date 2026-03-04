#include "validationInspector.h"
#include "../ui/colors.h"
#include "raymath.h"

namespace {
FillPattern makeBorderStyle(float patternSize) {
    FillPattern style;
    style.type = FillPatternType::Bordered;
    style.size = patternSize;
    style.useAccentColor = true;
    style.accentColor = Colors::Black;
    return style;
}
}

void ValidationInspector::validateAdjacency(const Mandala& mandala) {
    validatedWrongRegions.clear();

    std::vector<int> wrongRegionIds = verifyWrongRegions(mandala);
    const std::vector<Region>& regions = mandala.getRegions();
    validatedWrongRegions.reserve(wrongRegionIds.size());

    for (int regionId : wrongRegionIds) {
        if (regionId < 0 || regionId >= static_cast<int>(regions.size())) {
            continue;
        }
        validatedWrongRegions.emplace_back(regionId, regions[regionId].getColor());
    }

    validationOverlayEnabled = true;
}

void ValidationInspector::drawValidationOverlay(const Mandala& mandala, float cameraZoom) const {
    if (!validationOverlayEnabled) {
        return;
    }

    float effectiveZoom = std::max(cameraZoom, 0.001f);
    float worldBorderWidth = Clamp(8.0f / effectiveZoom, 2.5f, 20.0f);
    float borderPatternSize = Clamp(2.2f / effectiveZoom, 0.6f, 5.0f);

    const std::vector<Region>& regions = mandala.getRegions();
    const AdjacencyGraph& adjacencyGraph = mandala.getAdjacencyGraph();

    std::unordered_map<int, int> activeValidatedColors;
    activeValidatedColors.reserve(validatedWrongRegions.size());

    for (const auto& entry : validatedWrongRegions) {
        int regionId = entry.first;
        int validatedColor = entry.second;

        if (regionId < 0 || regionId >= static_cast<int>(regions.size())) {
            continue;
        }

        const Region& region = regions[regionId];
        if (!region.hasColor()) {
            continue;
        }

        if (region.getColor() != validatedColor) {
            continue;
        }

        activeValidatedColors.emplace(regionId, region.getColor());
    }

    for (const auto& activeEntry : activeValidatedColors) {
        int regionId = activeEntry.first;
        int regionColor = activeEntry.second;

        bool hasActiveConflict = false;
        const auto& neighbors = adjacencyGraph.getAdjacentRegions(regionId);
        for (int neighborId : neighbors) {
            auto neighborIt = activeValidatedColors.find(neighborId);
            if (neighborIt == activeValidatedColors.end()) {
                continue;
            }

            if (neighborIt->second == regionColor) {
                hasActiveConflict = true;
                break;
            }
        }

        if (!hasActiveConflict) {
            continue;
        }

        regions[regionId].drawWithColor(Colors::Transparent, Colors::Red, worldBorderWidth, makeBorderStyle(borderPatternSize));
    }
}

std::vector<int> ValidationInspector::verifyWrongRegions(const Mandala& mandala) const {
    std::vector<int> wrongRegionIds;
    const std::vector<Region>& regions = mandala.getRegions();
    const AdjacencyGraph& adjacencyGraph = mandala.getAdjacencyGraph();

    for (size_t i = 0; i < regions.size(); i++) {
        if (regions[i].hasColor()) {
            const auto& adjacentRegions = adjacencyGraph.getAdjacentRegions(i);
            for (int adjacentId : adjacentRegions) {
                if (regions[i].getColor() == regions[adjacentId].getColor()) {
                    wrongRegionIds.emplace_back(static_cast<int>(i));
                    break;
                }
            }
        }
    }
    return wrongRegionIds;
}
