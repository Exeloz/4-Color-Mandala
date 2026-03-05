#pragma once
#include <raylib.h>
#include <vector>

class ColorPalette {
public:
    static constexpr int LockedColorSlots = 1;
    static constexpr int NormalEditableColorCount = 4;
    static constexpr int MaxEditableColorCount = 10;

    ColorPalette();

    Color getColor(int index) const;
    const std::vector<Color>& getColors() const;
    int getColorCount() const;
    bool isValidColorIndex(int index) const;
    int getSelectedColorIndex() const;
    void setSelectedColorIndex(int index);
    void setColors(const std::vector<Color>& newColors);

private:
    std::vector<Color> colors;
    int selectedColorIndex;
};
