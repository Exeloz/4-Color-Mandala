#pragma once
#include "../game/gameState.h"
#include "../ui/button.h"
#include "../ui/colorWheelPicker.h"
#include "raylib.h"
#include <vector>

class PaletteScreen : public GameState {
public:
    PaletteScreen(const std::vector<Color>& initialPaletteColors = {});

    void update(float deltaTime) override;
    void draw() override;

    bool shouldTransitionToColoring() const;
    bool shouldReturnToSelection() const;
    bool consumePaletteChanged();
    const std::vector<Color>& getCustomizedColors() const;

private:
    std::vector<Color> paletteColors;
    int activeSlotIndex;
    ColorWheelPicker colorWheelPicker;

    Button backButton;
    Button continueButton;
    Button wheelAcceptButton;
    Button wheelCancelButton;

    bool transitionRequested;
    bool returnRequested;
    bool paletteChanged;

    bool wheelPopupVisible;
    int wheelPopupSlotIndex;

    void layoutControls();
    void layoutWheelPopup();
    void openWheelPopupForSlot(int slotIndex);
    void closeWheelPopup(bool applyChanges);

    Rectangle getPaletteSlotBounds(int slotIndex) const;
    Rectangle getWheelPopupBounds() const;
    Rectangle getWheelBoundsInPopup(const Rectangle& popupBounds) const;
};
