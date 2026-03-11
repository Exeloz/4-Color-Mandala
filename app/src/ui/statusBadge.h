#pragma once
#include "raylib.h"
#include <string>

// A small labeled badge shown in the top bar of the coloring screen.
// All badge types (HARD, MIN, DONE, rule badges, future badges) use this struct.
// If description is non-empty, tapping the badge shows a tooltip popup.
struct StatusBadge {
    std::string label;        // short uppercase text drawn on the badge
    std::string description;  // tooltip body (\n splits lines); empty = no tooltip

    Color bgColor       = {100, 100, 200, 255};
    Color bgColorOpen   = { 80,  20, 160, 255};  // when tooltip is open
    Color textColor     = {255, 255, 255, 255};
    Color borderColor   = {  0,   0,   0, 255};
};
