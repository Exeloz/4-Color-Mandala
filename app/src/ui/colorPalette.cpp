#include "colorPalette.h"
#include "colors.h"

constexpr int ColorPalette::LockedColorSlots;
constexpr int ColorPalette::NormalEditableColorCount;
constexpr int ColorPalette::MaxEditableColorCount;

ColorPalette::ColorPalette() : selectedColorIndex(0) {
    colors.push_back(Colors::None);
    colors.push_back(Colors::DarkBlue);
    colors.push_back(Colors::RoyalBlue);
    colors.push_back(Colors::DeepSkyBlue);
    colors.push_back(Colors::PaleTurquoise);
    colors.push_back(Colors::Orange);
    colors.push_back(Colors::Gold);
    colors.push_back(Colors::LimeGreen);
    colors.push_back(Colors::Violet);
    colors.push_back(Colors::Crimson);
    colors.push_back(Colors::SaddleBrown);
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
