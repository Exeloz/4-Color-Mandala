#include "colorWheelPopup.h"

#include "colors.h"
#include "raymath.h"
#include <algorithm>
#include <string>

ColorWheelPopup::ColorWheelPopup()
    : popup(),
      wheelPicker(),
      accepted(false),
      cancelled(false),
      activeSlotIndex(-1),
      selectedColor{0, 0, 0, 255},
      wheelBounds{0.0f, 0.0f, 0.0f, 0.0f} {
    popup.configure("Edit slot", "Apply", "Cancel");
    popup.setSizeRatios(0.86f, 0.82f);
}

void ColorWheelPopup::open(int slotIndex, const Color& initialColor) {
    if (slotIndex <= 0) {
        return;
    }

    activeSlotIndex = slotIndex;
    selectedColor = initialColor;
    accepted = false;
    cancelled = false;

    popup.configure("Edit slot " + std::to_string(slotIndex), "Apply", "Cancel");
    popup.show();
    updateLayout();
    wheelPicker.setColor(initialColor);
}

bool ColorWheelPopup::isVisible() const {
    return popup.isVisible();
}

void ColorWheelPopup::update() {
    if (!popup.isVisible()) {
        return;
    }

    popup.update();

    if (popup.consumeCancelled()) {
        cancelled = true;
        popup.hide();
        return;
    }

    if (popup.consumeConfirmed()) {
        selectedColor = wheelPicker.getSelectedColor();
        accepted = true;
        popup.hide();
        return;
    }

    updateLayout();
    wheelPicker.update();
}

void ColorWheelPopup::draw() {
    if (!popup.isVisible()) {
        return;
    }

    updateLayout();
    popup.drawShell();

    Color previewColor = wheelPicker.getSelectedColor();
    Rectangle popupBounds = popup.getBounds();

    float previewSize = std::max(34.0f, popupBounds.height * 0.11f);
    float previewX = popupBounds.x + popupBounds.width * 0.025f;
    float previewY = wheelBounds.y + (wheelBounds.height - previewSize) * 0.5f;
    Rectangle previewSwatch = {previewX, previewY, previewSize, previewSize};
    DrawRectangleRec(previewSwatch, previewColor);
    DrawRectangleLinesEx(previewSwatch, 2.0f, Colors::Black);

    int previewTextSize = std::max(15, static_cast<int>(popupBounds.height * 0.035f));
    const char* selectedText = "Preview";
    DrawText(selectedText,
             static_cast<int>(previewSwatch.x + (previewSwatch.width - MeasureText(selectedText, previewTextSize)) * 0.5f),
             static_cast<int>(previewSwatch.y + previewSwatch.height + 6.0f),
             previewTextSize,
             Colors::Black);

    wheelPicker.draw(Colors::WhiteSmoke);
    popup.drawButtons();
}

bool ColorWheelPopup::consumeAccepted(int& outSlotIndex, Color& outColor) {
    if (!accepted) {
        return false;
    }

    accepted = false;
    outSlotIndex = activeSlotIndex;
    outColor = selectedColor;
    return true;
}

bool ColorWheelPopup::consumeCancelled(int& outSlotIndex) {
    if (!cancelled) {
        return false;
    }

    cancelled = false;
    outSlotIndex = activeSlotIndex;
    return true;
}

void ColorWheelPopup::updateLayout() {
    Rectangle popupBounds = popup.getBounds();
    wheelBounds = popup.getContentBounds(0.08f, 0.18f, 0.24f);
    float centerShift = popupBounds.width * 0.05f;
    wheelBounds.x += centerShift;

    wheelPicker.setBounds(wheelBounds);
}
