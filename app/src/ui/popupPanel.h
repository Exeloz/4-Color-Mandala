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
    void setMaxSize(float maxWidth, float maxHeight);

    void show();
    void hide();
    bool isVisible() const;

    void refreshLayout();
    void updateButtons();
    void update();

    void drawShell() const;
    void drawButtons();

    bool consumeConfirmed();
    bool consumeCancelled();

    Rectangle getBounds() const;
    Rectangle getContentBounds(float horizontalPaddingRatio,
                               float topPaddingRatio,
                               float bottomPaddingRatio) const;

    Button& getConfirmButton();
    Button& getCancelButton();

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
