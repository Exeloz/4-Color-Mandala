#include "popupPanel.h"

#include "colors.h"
#include "raymath.h"
#include <algorithm>

PopupPanel::PopupPanel()
    : visible(false),
      confirmed(false),
      cancelled(false),
      title("Confirm"),
      panelBounds{0.0f, 0.0f, 0.0f, 0.0f},
      widthRatio(0.86f),
      heightRatio(0.82f),
      maxWidth(720.0f),
      maxHeight(720.0f),
      backdropAlpha(0.0f),
      confirmButton(0.0f, 0.0f, 130.0f, 52.0f, "Confirm"),
      cancelButton(0.0f, 0.0f, 130.0f, 52.0f, "Cancel") {}

void PopupPanel::configure(const std::string& newTitle,
                           const std::string& confirmLabel,
                           const std::string& cancelLabel) {
    title = newTitle;
    confirmButton = Button(0.0f, 0.0f, 130.0f, 52.0f, confirmLabel);
    cancelButton = Button(0.0f, 0.0f, 130.0f, 52.0f, cancelLabel);
}

void PopupPanel::setSizeRatios(float newWidthRatio, float newHeightRatio) {
    widthRatio = Clamp(newWidthRatio, 0.4f, 0.98f);
    heightRatio = Clamp(newHeightRatio, 0.35f, 0.98f);
}

void PopupPanel::setMaxSize(float newMaxWidth, float newMaxHeight) {
    maxWidth = std::max(120.0f, newMaxWidth);
    maxHeight = std::max(120.0f, newMaxHeight);
}

void PopupPanel::show() {
    visible = true;
    confirmed = false;
    cancelled = false;
    backdropAlpha = 0.0f;
    updateLayout();
}

void PopupPanel::hide() {
    visible = false;
    confirmed = false;
    cancelled = false;
    backdropAlpha = 0.0f;
}

bool PopupPanel::isVisible() const {
    return visible;
}

void PopupPanel::refreshLayout() {
    if (!visible) {
        return;
    }
    updateLayout();
}

void PopupPanel::updateButtons() {
    if (!visible) {
        return;
    }

    cancelButton.update();
    confirmButton.update();

    if (cancelButton.isClicked()) {
        cancelled = true;
        confirmed = false;
    }

    if (confirmButton.isClicked()) {
        confirmed = true;
        cancelled = false;
    }
}

void PopupPanel::update() {
    if (!visible) {
        return;
    }

    const float alphaLerp = std::min(1.0f, GetFrameTime() * 10.0f);
    backdropAlpha = Lerp(backdropAlpha, 1.0f, alphaLerp);

    refreshLayout();
    updateButtons();
}

void PopupPanel::drawShell() const {
    if (!visible) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Colors::Black, 0.55f * backdropAlpha));
    DrawRectangleRounded(panelBounds, 0.08f, 14, Fade(Colors::WhiteSmoke, backdropAlpha));
    DrawRectangleRoundedLinesEx(panelBounds, 0.08f, 14, 3.0f, Fade(Colors::DarkBlue, backdropAlpha));

    int titleSize = std::max(24, static_cast<int>(panelBounds.height * 0.07f));
    int titleWidth = MeasureText(title.c_str(), titleSize);
    int titleX = static_cast<int>(panelBounds.x + (panelBounds.width - titleWidth) * 0.5f);
    int titleY = static_cast<int>(panelBounds.y + panelBounds.height * 0.06f);
    DrawText(title.c_str(), titleX, titleY, titleSize, Fade(Colors::Black, backdropAlpha));
}

void PopupPanel::drawButtons() {
    if (!visible) {
        return;
    }

    cancelButton.draw();
    confirmButton.draw();
}

bool PopupPanel::consumeConfirmed() {
    const bool value = confirmed;
    confirmed = false;
    return value;
}

bool PopupPanel::consumeCancelled() {
    const bool value = cancelled;
    cancelled = false;
    return value;
}

Rectangle PopupPanel::getBounds() const {
    return panelBounds;
}

Rectangle PopupPanel::getContentBounds(float horizontalPaddingRatio,
                                       float topPaddingRatio,
                                       float bottomPaddingRatio) const {
    float horizontalPadding = panelBounds.width * horizontalPaddingRatio;
    float topPadding = panelBounds.height * topPaddingRatio;
    float bottomPadding = panelBounds.height * bottomPaddingRatio;
    return {
        panelBounds.x + horizontalPadding,
        panelBounds.y + topPadding,
        panelBounds.width - (2.0f * horizontalPadding),
        panelBounds.height - topPadding - bottomPadding
    };
}

Button& PopupPanel::getConfirmButton() {
    return confirmButton;
}

Button& PopupPanel::getCancelButton() {
    return cancelButton;
}

void PopupPanel::updateLayout() {
    float dialogWidth = std::min(maxWidth, GetScreenWidth() * widthRatio);
    float dialogHeight = std::min(maxHeight, GetScreenHeight() * heightRatio);
    panelBounds = {
        (GetScreenWidth() - dialogWidth) * 0.5f,
        (GetScreenHeight() - dialogHeight) * 0.5f,
        dialogWidth,
        dialogHeight,
    };

    float buttonWidth = std::max(120.0f, dialogWidth * 0.24f);
    float buttonHeight = std::max(48.0f, dialogHeight * 0.13f);
    float spacing = std::max(14.0f, dialogWidth * 0.05f);
    float buttonsY = panelBounds.y + dialogHeight - buttonHeight - std::max(18.0f, dialogHeight * 0.06f);
    float startX = panelBounds.x + (dialogWidth - (buttonWidth * 2.0f + spacing)) * 0.5f;

    cancelButton.setPosition(startX, buttonsY);
    cancelButton.setSize(buttonWidth, buttonHeight);

    confirmButton.setPosition(startX + buttonWidth + spacing, buttonsY);
    confirmButton.setSize(buttonWidth, buttonHeight);
}
