#pragma once

#include "colorWheelPicker.h"
#include "popupPanel.h"

class ColorWheelPopup {
public:
    ColorWheelPopup();

    void open(int slotIndex, const Color& initialColor);
    bool isVisible() const;

    void update();
    void draw();

    bool consumeAccepted(int& outSlotIndex, Color& outColor);
    bool consumeCancelled(int& outSlotIndex);

private:
    void updateLayout();

    PopupPanel popup;
    ColorWheelPicker wheelPicker;

    bool accepted;
    bool cancelled;

    int activeSlotIndex;
    Color selectedColor;
    Rectangle wheelBounds;
};
