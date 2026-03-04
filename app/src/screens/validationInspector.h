#pragma once

#include "../mandala/mandala.h"
#include "../rendering/fillPattern.h"
#include "raylib.h"
#include <unordered_map>
#include <utility>
#include <vector>

class ValidationInspector {
public:
    void validateAdjacency(const Mandala& mandala);
    void drawValidationOverlay(const Mandala& mandala, float cameraZoom) const;

private:
    std::vector<int> verifyWrongRegions(const Mandala& mandala) const;

    bool validationOverlayEnabled = false;
    std::vector<std::pair<int, int>> validatedWrongRegions;
};
