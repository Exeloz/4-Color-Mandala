#pragma once

#include "popupPanel.h"
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
    std::string message;
    PopupPanel popup;
};
