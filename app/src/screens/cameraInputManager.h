#pragma once

#include "raylib.h"
#include <functional>

class CameraInputManager {
public:
    CameraInputManager(float minZoom, float maxZoom, float zoomStep, float touchPanStartThreshold, float touchPinchSensitivity);

    void update(Camera2D& camera, float& zoom, const std::function<bool(Vector2)>& isPointerOverUi);
    bool isDraggingCamera() const;

private:
    float minZoom;
    float maxZoom;
    float zoomStep;
    float touchPanStartThreshold;
    float touchPinchSensitivity;

    bool isPanning;
    bool isDragging;
    bool isPinching;
    float lastPinchDistance;
    Vector2 lastPanPointer;
};
