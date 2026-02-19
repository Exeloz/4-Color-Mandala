#include "cameraInputManager.h"
#include "../ui/input.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

CameraInputManager::CameraInputManager(float minZoom, float maxZoom, float zoomStep, float touchPanStartThreshold, float touchPinchSensitivity)
    : minZoom(minZoom),
      maxZoom(maxZoom),
      zoomStep(zoomStep),
      touchPanStartThreshold(touchPanStartThreshold),
      touchPinchSensitivity(touchPinchSensitivity),
      isPanning(false),
      isDragging(false),
      isPinching(false),
      lastPinchDistance(0.0f),
      lastPanPointer{} {}

void CameraInputManager::update(Camera2D& camera, float& zoom, const std::function<bool(Vector2)>& isPointerOverUi) {
    camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};

    float zoomDelta = 0.0f;
    Vector2 zoomAnchor = Input::GetPointerPosition();

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        zoomDelta += wheel * zoomStep;
    }

    bool isTwoFingerGesture = false;
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    int touchCount = GetTouchPointCount();
    if (touchCount >= 2) {
        isTwoFingerGesture = true;
        isDragging = true;

        Vector2 touchA = GetTouchPosition(0);
        Vector2 touchB = GetTouchPosition(1);
        Vector2 midpoint = {(touchA.x + touchB.x) * 0.5f, (touchA.y + touchB.y) * 0.5f};
        float currentPinchDistance = Vector2Distance(touchA, touchB);
        zoomAnchor = midpoint;

        if (!isPinching) {
            isPinching = true;
            lastPinchDistance = currentPinchDistance;
            lastPanPointer = midpoint;
        } else {
            Vector2 delta = Vector2Subtract(midpoint, lastPanPointer);
            camera.target = Vector2Subtract(camera.target, Vector2Scale(delta, 1.0f / camera.zoom));

            Vector2 screenSize = {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
            float screenDiagonal = Vector2Length(screenSize);
            float normalizedPinchDelta = (currentPinchDistance - lastPinchDistance) / std::max(1.0f, screenDiagonal);
            zoomDelta += normalizedPinchDelta * touchPinchSensitivity;

            lastPanPointer = midpoint;
            lastPinchDistance = currentPinchDistance;
        }
    } else {
        isPinching = false;
        lastPinchDistance = 0.0f;
    }
#endif

    if (IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT)) {
        zoomDelta -= zoomStep * GetFrameTime();
    }
    if (IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD)) {
        zoomDelta += zoomStep * GetFrameTime();
    }

    if (zoomDelta != 0.0f) {
        Vector2 worldBeforeZoom = GetScreenToWorld2D(zoomAnchor, camera);

        zoom = Clamp(zoom + zoomDelta, minZoom, maxZoom);
        camera.zoom = zoom;

        Vector2 worldAfterZoom = GetScreenToWorld2D(zoomAnchor, camera);
        camera.target = Vector2Add(camera.target, Vector2Subtract(worldBeforeZoom, worldAfterZoom));
    }

    if (isTwoFingerGesture) {
        isPanning = false;
        return;
    }

    bool shouldPan = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
                     (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    Vector2 pointerPos = Input::GetPointerPosition();

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    int touchCount = GetTouchPointCount();
    if (touchCount == 1) {
        pointerPos = GetTouchPosition(0);
        shouldPan = !isPointerOverUi(pointerPos);
    } else {
        shouldPan = false;
    }
#endif

    if (shouldPan) {
        if (!isPanning) {
            isPanning = true;
            isDragging = false;
            lastPanPointer = pointerPos;
            return;
        }

        Vector2 delta = Vector2Subtract(pointerPos, lastPanPointer);
        float deltaLength = Vector2Length(delta);

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
        if (isDragging || deltaLength >= touchPanStartThreshold) {
            camera.target = Vector2Subtract(camera.target, Vector2Scale(delta, 1.0f / camera.zoom));
            isDragging = true;
        }
#else
        if (deltaLength > 0.0f) {
            camera.target = Vector2Subtract(camera.target, Vector2Scale(delta, 1.0f / camera.zoom));
            isDragging = true;
        }
#endif

        lastPanPointer = pointerPos;
    } else {
        isPanning = false;
        isDragging = false;
    }
}

bool CameraInputManager::isDraggingCamera() const {
    return isDragging;
}
