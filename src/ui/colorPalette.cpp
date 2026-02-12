#include "colorPalette.h"

ColorPalette::ColorPalette() : selectedColorIndex(0) {
    colors.push_back({255, 0, 0, 255});
    colors.push_back({0, 0, 255, 255});
    colors.push_back({255, 255, 0, 255});
    colors.push_back({0, 255, 0, 255});
    colors.push_back({255, 255, 255, 255});
}

Color ColorPalette::getColor(int index) const {
    if (isValidColorIndex(index)) {
        return colors[index];
    }
    return {0, 0, 0, 255};
}

int ColorPalette::getColorCount() const {
    return colors.size();
}

bool ColorPalette::isValidColorIndex(int index) const {
    return index >= 0 && index < static_cast<int>(colors.size());
}

int ColorPalette::getSelectedColorIndex() const {
    return selectedColorIndex;
}

void ColorPalette::setSelectedColorIndex(int index) {
    if (isValidColorIndex(index)) {
        selectedColorIndex = index;
    }
}
