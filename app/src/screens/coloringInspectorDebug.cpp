#include "coloringInspector.h"

#include "../ui/colors.h"
#include "../ui/input.h"

#include <algorithm>

void ColoringInspector::updateDebug(Mandala& mandala, const Camera2D& camera, Camera2D& mutableCamera) {
    int previousInspectRegionId = debugInspectRegionId;

    if (IsKeyPressed(KEY_F3)) {
        debugAdjacencyMode = !debugAdjacencyMode;
        if (!debugAdjacencyMode) {
            debugInspectRegionId = -1;
            debugHoverRegionId = -1;
        }
    }

    if (!debugAdjacencyMode) {
        return;
    }

    int arrowDirection = 0;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP)) {
        arrowDirection = 1;
    } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_DOWN)) {
        arrowDirection = -1;
    }

    if (arrowDirection != 0) {
        const auto& regions = mandala.getRegions();
        std::vector<int> sortedIds = collectSortedRegionIds(regions);
        debugInspectRegionId = cycleRegionId(sortedIds, debugInspectRegionId, arrowDirection);
    }

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    debugHoverRegionId = getRegionIdAtWorldPosition(mandala, worldPos);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && debugHoverRegionId >= 0) {
        debugInspectRegionId = debugHoverRegionId;
    }

    if (IsKeyPressed(KEY_C)) {
        debugInspectRegionId = -1;
    }

    if (debugInspectRegionId >= 0 && debugHoverRegionId >= 0 && debugInspectRegionId != debugHoverRegionId) {
        if (IsKeyPressed(KEY_A)) {
            logAdjacencySuggestion(mandala, true, debugInspectRegionId, debugHoverRegionId);
        }
        if (IsKeyPressed(KEY_R)) {
            logAdjacencySuggestion(mandala, false, debugInspectRegionId, debugHoverRegionId);
        }
    }

    if (IsKeyPressed(KEY_B)) {
        int targetRegionId = debugInspectRegionId >= 0 ? debugInspectRegionId : debugHoverRegionId;
        if (targetRegionId >= 0) {
            logRegionBlackoutSuggestion(mandala, targetRegionId);
        }
    }

    if (debugInspectRegionId >= 0 && debugInspectRegionId != previousInspectRegionId) {
        centerCameraOnRegion(mandala, debugInspectRegionId, mutableCamera);
    }
}

void ColoringInspector::logAdjacencySuggestion(const Mandala& mandala, bool shouldExist, int regionA, int regionB) {
    int a = std::min(regionA, regionB);
    int b = std::max(regionA, regionB);

    bool currentlyAdjacent = mandala.getAdjacencyGraph().areAdjacent(a, b);

    if (shouldExist) {
        TraceLog(LOG_INFO, "[ADJ DEBUG] Suggest ADD (%d, %d)", a, b);
        if (currentlyAdjacent) {
            TraceLog(LOG_INFO, "[ADJ DEBUG] Already adjacent in current graph.");
        }
        TraceLog(LOG_INFO, "[ADJ DEBUG] Line to add: adjacencyGraph.addAdjacency(%d, %d);", a, b);
        if (applyAdjacencyJsonEdit(mandala, true, a, b)) {
            TraceLog(LOG_INFO, "[ADJ DEBUG] Updated adjacency JSON on disk for mandala %d.", mandala.getId());
        } else {
            TraceLog(LOG_WARNING, "[ADJ DEBUG] Failed to update adjacency JSON on disk for mandala %d.", mandala.getId());
        }
        return;
    }

    TraceLog(LOG_INFO, "[ADJ DEBUG] Suggest REMOVE (%d, %d)", a, b);
    if (!currentlyAdjacent) {
        TraceLog(LOG_INFO, "[ADJ DEBUG] Pair not currently adjacent in graph.");
    }
    TraceLog(LOG_INFO, "[ADJ DEBUG] Remove pair from adjacency JSON: [%d, %d]", a, b);
    if (applyAdjacencyJsonEdit(mandala, false, a, b)) {
        TraceLog(LOG_INFO, "[ADJ DEBUG] Updated adjacency JSON on disk for mandala %d.", mandala.getId());
    } else {
        TraceLog(LOG_WARNING, "[ADJ DEBUG] Failed to update adjacency JSON on disk for mandala %d.", mandala.getId());
    }
}

void ColoringInspector::logRegionBlackoutSuggestion(Mandala& mandala, int regionId) {
    TraceLog(LOG_INFO, "[REGION DEBUG] Set region %d defaultColor=black and colorable=false", regionId);

    Region* region = mandala.getRegionById(regionId);
    if (region != nullptr) {
        region->setDefaultColor(Colors::Black);
        region->setColor(-1);
        region->setColorable(false);
        TraceLog(LOG_INFO, "[REGION DEBUG] Applied immediate in-memory blackout for region %d.", regionId);
    } else {
        TraceLog(LOG_WARNING, "[REGION DEBUG] Region %d not found in current mandala instance.", regionId);
    }

    if (applyRegionBlackoutJsonEdit(mandala, regionId)) {
        TraceLog(LOG_INFO, "[REGION DEBUG] Updated regions JSON on disk for mandala %d.", mandala.getId());
    } else {
        TraceLog(LOG_WARNING, "[REGION DEBUG] Failed to update regions JSON on disk for mandala %d.", mandala.getId());
    }

    if (applyAdjacencyJsonRemoveAllForRegion(mandala, regionId)) {
        TraceLog(LOG_INFO, "[REGION DEBUG] Removed all adjacency pairs containing region %d in JSON.", regionId);
    } else {
        TraceLog(LOG_WARNING, "[REGION DEBUG] Failed to clear adjacency JSON pairs for region %d.", regionId);
    }
}
