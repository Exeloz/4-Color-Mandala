#include "colorPalette.h"
#include "colors.h"

ColorPalette::ColorPalette() : selectedColorIndex(0) {
    colors.push_back(Colors::None);
    colors.push_back(Colors::DarkBlue);
    colors.push_back(Colors::RoyalBlue);
    colors.push_back(Colors::DeepSkyBlue);
    colors.push_back(Colors::PaleTurquoise);
}

Color ColorPalette::getColor(int index) const {
    if (isValidColorIndex(index)) {
        return colors[index];
    }
    return {0, 0, 0, 255};
}

const std::vector<Color>& ColorPalette::getColors() const {
    return colors;
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

void ColorPalette::setColors(const std::vector<Color>& newColors) {
    if (newColors.empty()) {
        return;
    }

    colors = newColors;
    if (!isValidColorIndex(selectedColorIndex)) {
        selectedColorIndex = 0;
    }
}
