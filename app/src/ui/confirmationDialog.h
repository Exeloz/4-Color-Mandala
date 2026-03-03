#pragma once

#include "button.h"
#include <string>

class ConfirmationDialog {
public:
    ConfirmationDialog();

    void configure(const std::string& title,
                   const std::string& message,
                   const std::string& confirmLabel = "Confirm",
                   const std::string& cancelLabel = "Cancel");
    void show();
    void hide();
    bool isVisible() const;

    void update();
    void draw();

    bool consumeConfirmed();
    bool consumeCancelled();

private:
    void updateLayout();

    bool visible;
    bool confirmed;
    bool cancelled;
    std::string title;
    std::string message;
    Rectangle panelBounds;
    Button confirmButton;
    Button cancelButton;
};
