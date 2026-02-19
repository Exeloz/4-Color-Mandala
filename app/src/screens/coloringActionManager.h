#pragma once

#include "../mandala/mandala.h"
#include <vector>

struct ColoringAction {
    int regionId;
    int previousColor;
    int newColor;
};

class ColoringActionManager {
public:
    bool applyColorChange(Mandala& mandala, int regionId, int newColor);
    bool undoLast(Mandala& mandala);
    bool canUndo() const;
    void clear();

private:
    std::vector<ColoringAction> history;
};
