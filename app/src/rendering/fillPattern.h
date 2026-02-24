#pragma once

#include <raylib.h>

enum class FillPatternType {
    Solid,
    Striped,
    Dotted,
    Bordered
};

struct FillPattern {
    FillPatternType type = FillPatternType::Solid;
    Color accentColor = BLANK;
    float size = 1.0f;
    bool useAccentColor = false;
};
