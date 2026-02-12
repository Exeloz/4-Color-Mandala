#pragma once
#include "raylib.h"

namespace Input {
    inline Vector2 GetPointerPosition() {
        #if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
        if (GetTouchPointCount() > 0) {
            return GetTouchPosition(0);
        }
        #endif
        return GetMousePosition();
    }

    inline bool IsPointerPressed() {
        #if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
        if (IsGestureDetected(GESTURE_TAP)) {
            return true;
        }
        #endif
        return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }

    inline bool IsPointerDown() {
        #if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
        if (GetTouchPointCount() > 0) {
            return true;
        }
        #endif
        return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    }
}
