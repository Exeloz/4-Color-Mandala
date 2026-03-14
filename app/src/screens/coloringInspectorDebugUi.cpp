#include "coloringInspector.h"

#include "../ui/colors.h"

#include <sstream>

void ColoringInspector::drawDebugOverlay(const Mandala& mandala) const {
    if (!debugAdjacencyMode || debugInspectRegionId < 0) {
        return;
    }

    const Region* selectedRegion = mandala.getRegionById(debugInspectRegionId);
    if (selectedRegion == nullptr) {
        return;
    }

    selectedRegion->drawWithColor(Fade(Colors::Cyan, 0.45f), Colors::DarkCyan, 6.0f);

    const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
    for (int neighborId : neighbors) {
        const Region* neighborRegion = mandala.getRegionById(neighborId);
        if (neighborRegion == nullptr) {
            continue;
        }

        neighborRegion->drawWithColor(Fade(Colors::Blue, 0.30f), Colors::Blue, 4.0f);
    }

    if (debugHoverRegionId >= 0 && debugHoverRegionId != debugInspectRegionId) {
        const Region* hoverRegion = mandala.getRegionById(debugHoverRegionId);
        if (hoverRegion != nullptr) {
            hoverRegion->drawWithColor(Fade(Colors::LightSkyBlue, 0.35f), Colors::DodgerBlue, 2.0f);
        }
    }
}

void ColoringInspector::drawDebugInfoPanel(const Mandala& mandala, float uiScale) const {
    if (!debugAdjacencyMode) {
        return;
    }

    int infoX = static_cast<int>(15.0f * uiScale);
    int infoY = static_cast<int>(70.0f * uiScale);
    int infoW = static_cast<int>(760.0f * uiScale);
    int infoH = static_cast<int>(58.0f * uiScale);
    DrawRectangle(infoX, infoY, infoW, infoH, Fade(Colors::White, 0.85f));
    DrawRectangleLines(infoX, infoY, infoW, infoH, Colors::DarkGray);

    std::ostringstream info;
    info << "DEBUG ADJ: ON  |  Hover: " << debugHoverRegionId
         << "  |  Inspect (Right Click): " << debugInspectRegionId
         << "  |  Clear Inspect: C  |  A=Add  R=Remove  B=Black+Lock";
    DrawText(info.str().c_str(),
             static_cast<int>(24.0f * uiScale),
             static_cast<int>(82.0f * uiScale),
             static_cast<int>(20.0f * uiScale),
             Colors::Black);

    if (debugInspectRegionId >= 0) {
        const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(debugInspectRegionId);
        std::ostringstream neighborText;
        neighborText << "Neighbors(" << neighbors.size() << "): ";

        bool first = true;
        for (int id : neighbors) {
            if (!first) {
                neighborText << ", ";
            }
            neighborText << id;
            first = false;
        }

        DrawText(neighborText.str().c_str(),
                 static_cast<int>(24.0f * uiScale),
                 static_cast<int>(103.0f * uiScale),
                 static_cast<int>(18.0f * uiScale),
                 Colors::DarkBlue);
    }
}

bool ColoringInspector::isDebugAdjacencyMode() const {
    return debugAdjacencyMode;
}

int ColoringInspector::getDebugInspectRegionId() const {
    return debugInspectRegionId;
}

int ColoringInspector::getDebugHoverRegionId() const {
    return debugHoverRegionId;
}
