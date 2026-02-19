#include "coloringActionManager.h"

bool ColoringActionManager::applyColorChange(Mandala& mandala, int regionId, int newColor) {
    Region* region = mandala.getRegionById(regionId);
    if (region == nullptr || !region->isColorable()) {
        return false;
    }

    int previousColor = region->getColor();
    if (previousColor == newColor) {
        return false;
    }

    region->setColor(newColor);
    history.push_back({regionId, previousColor, newColor});
    return true;
}

bool ColoringActionManager::undoLast(Mandala& mandala) {
    if (history.empty()) {
        return false;
    }

    ColoringAction action = history.back();
    history.pop_back();

    Region* region = mandala.getRegionById(action.regionId);
    if (region == nullptr || !region->isColorable()) {
        return false;
    }

    region->setColor(action.previousColor);
    return true;
}

bool ColoringActionManager::canUndo() const {
    return !history.empty();
}

void ColoringActionManager::clear() {
    history.clear();
}
