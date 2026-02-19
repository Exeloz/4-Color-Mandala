#pragma once

#include "raylib.h"
#include <vector>

class ColorCatalog {
public:
    static const std::vector<Color>& getAvailableColors();
};
