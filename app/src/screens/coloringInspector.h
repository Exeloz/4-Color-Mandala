#pragma once

#include "../mandala/mandala.h"
#include "validationInspector.h"
#include "raylib.h"
#include <set>
#include <utility>
#include <vector>

class ColoringInspector {
public:
    void enterAnalysisMode();
    void exitAnalysisMode();
    void clearAnalysisSelection();
    bool isAnalysisMode() const;

    void validateAdjacency(const Mandala& mandala);
    void drawValidationOverlay(const Mandala& mandala) const;

    void updateAnalysis(const Mandala& mandala, const Camera2D& camera, bool pointerOverUi, bool isDraggingCamera);
    void updateDebug(const Mandala& mandala, const Camera2D& camera, Camera2D& mutableCamera);

    void drawAnalysisOverlay(const Mandala& mandala, const std::vector<Color>& colorPalette) const;
    void drawDebugOverlay(const Mandala& mandala) const;
    void drawDebugInfoPanel(const Mandala& mandala, float uiScale) const;

    bool isDebugAdjacencyMode() const;
    int getDebugInspectRegionId() const;
    int getDebugHoverRegionId() const;

private:
    bool analysisMode = false;
    int analysisInspectRegionId = -1;
    int analysisHoverRegionId = -1;

    bool debugAdjacencyMode = false;
    int debugInspectRegionId = -1;
    int debugHoverRegionId = -1;
    std::set<std::pair<int, int>> debugSuggestedAdds;
    std::set<std::pair<int, int>> debugSuggestedRemoves;

    ValidationInspector validationInspector;
    static std::vector<int> collectSortedRegionIds(const std::vector<Region>& regions);
    static int cycleRegionId(const std::vector<int>& sortedIds, int currentId, int direction);
    int getRegionIdAtWorldPosition(const Mandala& mandala, Vector2 worldPos) const;
    void centerCameraOnRegion(const Mandala& mandala, int regionId, Camera2D& camera) const;
    void logAdjacencySuggestion(const Mandala& mandala, bool shouldExist, int regionA, int regionB);
    bool applyAdjacencyJsonEdit(int mandalaId, bool shouldExist, int regionA, int regionB) const;
    static bool resolveAdjacencyPathForMandala(int mandalaId, std::string& adjacencyPath);
    static bool readFileText(const std::string& path, std::string& content);
    static bool writeFileText(const std::string& path, const std::string& content);
};
