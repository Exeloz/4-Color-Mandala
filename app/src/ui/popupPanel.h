#pragma once

#include "button.h"
#include "raylib.h"
#include <string>

class PopupPanel {
public:
    PopupPanel();

    void configure(const std::string& title,
                   const std::string& confirmLabel = "Confirm",
                   const std::string& cancelLabel = "Cancel");

    void setSizeRatios(float widthRatio, float heightRatio);

    void show();
    void hide();
    bool isVisible() const;

    void update();

    void drawShell() const;
    void drawButtons();

    bool consumeConfirmed();
    bool consumeCancelled();

    Rectangle getBounds() const;
    Rectangle getContentBounds(float horizontalPaddingRatio,
                               float topPaddingRatio,
                               float bottomPaddingRatio) const;

private:
    void updateLayout();

    bool visible;
    bool confirmed;
    bool cancelled;

    std::string title;
    Rectangle panelBounds;

    float widthRatio;
    float heightRatio;
    float maxWidth;
    float maxHeight;

    Button confirmButton;
    Button cancelButton;
};
