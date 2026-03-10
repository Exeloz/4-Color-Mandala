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
            currentColor{0, 0, 0, 255},
      selectedColor{0, 0, 0, 255},
            wheelBounds{0.0f, 0.0f, 0.0f, 0.0f},
            colorColumnBounds{0.0f, 0.0f, 0.0f, 0.0f} {
    popup.configure("Edit slot", "Apply", "Cancel");
        popup.setSizeRatios(0.96f, 0.98f);
        popup.setMaxSize(10000.0f, 10000.0f);
}

void ColorWheelPopup::open(int slotIndex, const Color& initialColor) {
    if (slotIndex <= 0) {
        return;
    }

    activeSlotIndex = slotIndex;
    currentColor = initialColor;
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

    popup.refreshLayout();
    updateActionButtonsLayout();
    popup.updateButtons();

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

    Rectangle popupBounds = popup.getBounds();
    drawColorColumn(popupBounds);

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

    float contentTop = popupBounds.y + popupBounds.height * 0.16f;
    float contentBottom = popupBounds.y + popupBounds.height * 0.90f;
    float contentHeight = std::max(120.0f, contentBottom - contentTop);

    float sidePadding = popupBounds.width * 0.03f;
    float interGap = popupBounds.width * 0.02f;

    float sideButtonWidth = std::max(110.0f, popupBounds.width * 0.12f);
    float colorColumnWidth = std::max(90.0f, popupBounds.width * 0.12f);

    float wheelStartX = popupBounds.x + sidePadding + sideButtonWidth + interGap;
    float wheelWidth = popupBounds.width - (2.0f * sidePadding)
                      - (2.0f * sideButtonWidth)
                      - colorColumnWidth
                      - (3.0f * interGap);
    wheelWidth = std::max(220.0f, wheelWidth);

    wheelBounds = {wheelStartX, contentTop, wheelWidth, contentHeight};
    colorColumnBounds = {wheelStartX + wheelWidth + interGap, contentTop, colorColumnWidth, contentHeight};

    wheelPicker.setBounds(wheelBounds);
}

void ColorWheelPopup::updateActionButtonsLayout() {
    Rectangle popupBounds = popup.getBounds();
    Button& cancelButton = popup.getCancelButton();
    Button& confirmButton = popup.getConfirmButton();

    float sidePadding = popupBounds.width * 0.03f;
    float sideButtonWidth = std::max(110.0f, popupBounds.width * 0.12f);
    float sideButtonHeight = std::max(60.0f, popupBounds.height * 0.14f);
    float buttonY = popupBounds.y + (popupBounds.height - sideButtonHeight) * 0.5f;

    cancelButton.setPosition(popupBounds.x + sidePadding, buttonY);
    cancelButton.setSize(sideButtonWidth, sideButtonHeight);
    cancelButton.setColors(Color{180, 45, 45, 255}, Color{210, 70, 70, 255}, Colors::Black, Colors::White);

    confirmButton.setPosition(popupBounds.x + popupBounds.width - sidePadding - sideButtonWidth, buttonY);
    confirmButton.setSize(sideButtonWidth, sideButtonHeight);
    confirmButton.setColors(Color{46, 150, 65, 255}, Color{72, 181, 92, 255}, Colors::Black, Colors::White);
}

void ColorWheelPopup::drawColorColumn(const Rectangle& popupBounds) const {
    Color previewColor = wheelPicker.getSelectedColor();

    float swatchSize = std::min(colorColumnBounds.width, colorColumnBounds.height * 0.26f);
    swatchSize = std::max(56.0f, swatchSize);

    float topSwatchY = colorColumnBounds.y + colorColumnBounds.height * 0.15f;
    float bottomSwatchY = colorColumnBounds.y + colorColumnBounds.height * 0.56f;
    float swatchX = colorColumnBounds.x + (colorColumnBounds.width - swatchSize) * 0.5f;

    Rectangle currentSwatch = {swatchX, topSwatchY, swatchSize, swatchSize};
    Rectangle previewSwatch = {swatchX, bottomSwatchY, swatchSize, swatchSize};

    DrawRectangleRounded(currentSwatch, 0.18f, 8, currentColor);
    DrawRectangleRoundedLinesEx(currentSwatch, 0.18f, 8, 2.0f, Colors::DarkGray);
    DrawRectangleRounded(previewSwatch, 0.18f, 8, previewColor);
    DrawRectangleRoundedLinesEx(previewSwatch, 0.18f, 8, 2.0f, Colors::Black);

    int labelSize = std::max(14, static_cast<int>(popupBounds.height * 0.03f));
    const char* currentLabel = "Current";
    const char* previewLabel = "Preview";

    DrawText(currentLabel,
             static_cast<int>(currentSwatch.x + (currentSwatch.width - MeasureText(currentLabel, labelSize)) * 0.5f),
             static_cast<int>(currentSwatch.y - labelSize - 6.0f),
             labelSize,
             Colors::DarkSlateGray);

    DrawText(previewLabel,
             static_cast<int>(previewSwatch.x + (previewSwatch.width - MeasureText(previewLabel, labelSize)) * 0.5f),
             static_cast<int>(previewSwatch.y - labelSize - 6.0f),
             labelSize,
             Colors::Black);
}
