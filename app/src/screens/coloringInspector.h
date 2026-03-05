#pragma once

#include "../mandala/mandala.h"
#include "validationInspector.h"
#include "raylib.h"
#include <string>
#include <vector>

class ColoringInspector {
public:
    void enterAnalysisMode();
    void exitAnalysisMode();
    void clearAnalysisSelection();
    bool isAnalysisMode() const;

    void validateAdjacency(const Mandala& mandala);
    void drawValidationOverlay(const Mandala& mandala, float cameraZoom) const;

    void updateAnalysis(const Mandala& mandala, const Camera2D& camera, bool pointerOverUi, bool isDraggingCamera);
    void updateDebug(Mandala& mandala, const Camera2D& camera, Camera2D& mutableCamera, bool hardModeEnabled);

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

    ValidationInspector validationInspector;
    static std::vector<int> collectSortedRegionIds(const std::vector<Region>& regions);
    static int cycleRegionId(const std::vector<int>& sortedIds, int currentId, int direction);
    int getRegionIdAtWorldPosition(const Mandala& mandala, Vector2 worldPos) const;
    void centerCameraOnRegion(const Mandala& mandala, int regionId, Camera2D& camera) const;
    void logAdjacencySuggestion(const Mandala& mandala,
                                bool shouldExist,
                                int regionA,
                                int regionB,
                                bool hardModeEnabled);
    void logRegionBlackoutSuggestion(Mandala& mandala, int regionId, bool hardModeEnabled);
    bool applyAdjacencyJsonEdit(int mandalaId, bool shouldExist, int regionA, int regionB, bool hardModeEnabled) const;
    bool applyAdjacencyJsonRemoveAllForRegion(int mandalaId, int regionId, bool hardModeEnabled) const;
    bool applyRegionBlackoutJsonEdit(int mandalaId, int regionId) const;
    static bool resolveAdjacencyPathForMandala(int mandalaId, bool hardModeEnabled, std::string& adjacencyPath);
    static bool resolveRegionsPathForMandala(int mandalaId, std::string& regionsPath);
    static bool readFileText(const std::string& path, std::string& content);
    static bool writeFileText(const std::string& path, const std::string& content);
};
