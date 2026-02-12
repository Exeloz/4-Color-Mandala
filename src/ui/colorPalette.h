#pragma once
#include <raylib.h>
#include <vector>

class ColorPalette {
public:
    ColorPalette();

    Color getColor(int index) const;
    const std::vector<Color>& getColors() const;
    int getColorCount() const;
    bool isValidColorIndex(int index) const;
    int getSelectedColorIndex() const;
    void setSelectedColorIndex(int index);

private:
    std::vector<Color> colors;
    int selectedColorIndex;
};
