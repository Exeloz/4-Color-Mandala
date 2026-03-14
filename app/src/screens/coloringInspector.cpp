#include "coloringInspector.h"

void ColoringInspector::validateAdjacency(const Mandala& mandala) {
    validationInspector.validateAdjacency(mandala);
}

void ColoringInspector::drawValidationOverlay(const Mandala& mandala, float cameraZoom) const {
    validationInspector.drawValidationOverlay(mandala, cameraZoom);
}
