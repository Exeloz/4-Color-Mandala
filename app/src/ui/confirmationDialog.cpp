#include "confirmationDialog.h"

#include "colors.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace {
std::string trimToEllipsis(const std::string& text, int textSize, float maxWidth) {
    const std::string ellipsis = "...";
    if (MeasureText(text.c_str(), textSize) <= maxWidth) {
        return text;
    }

    std::string candidate = text;
    while (!candidate.empty()) {
        candidate.pop_back();
        std::string attempt = candidate + ellipsis;
        if (MeasureText(attempt.c_str(), textSize) <= maxWidth) {
            return attempt;
        }
    }

    return ellipsis;
}

std::vector<std::string> wrapText(const std::string& text, int textSize, float maxWidth, int maxLines) {
    std::vector<std::string> words;
    std::stringstream stream(text);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }

    if (words.empty()) {
        return {""};
    }

    std::vector<std::string> lines;
    std::string current;

    for (size_t i = 0; i < words.size(); i++) {
        const std::string& nextWord = words[i];
        std::string attempt = current.empty() ? nextWord : (current + " " + nextWord);

        if (MeasureText(attempt.c_str(), textSize) <= maxWidth) {
            current = attempt;
            continue;
        }

        if (current.empty()) {
            current = trimToEllipsis(nextWord, textSize, maxWidth);
        }

        lines.push_back(current);
        current.clear();

        if (static_cast<int>(lines.size()) == maxLines - 1) {
            std::string remaining = nextWord;
            for (size_t j = i + 1; j < words.size(); j++) {
                remaining += " " + words[j];
            }
            lines.push_back(trimToEllipsis(remaining, textSize, maxWidth));
            return lines;
        }

        i--;
    }

    if (!current.empty()) {
        lines.push_back(current);
    }

    if (static_cast<int>(lines.size()) > maxLines) {
        lines.resize(maxLines);
        lines.back() = trimToEllipsis(lines.back(), textSize, maxWidth);
    }

    return lines;
}
}

ConfirmationDialog::ConfirmationDialog()
    : visible(false),
      confirmed(false),
      cancelled(false),
      title("Confirm"),
      message("Are you sure?"),
      panelBounds{0.0f, 0.0f, 0.0f, 0.0f},
      confirmButton(0.0f, 0.0f, 120.0f, 52.0f, "Confirm"),
      cancelButton(0.0f, 0.0f, 120.0f, 52.0f, "Cancel") {
    confirmButton.setTextScale(1.0f);
    cancelButton.setTextScale(1.0f);
}

void ConfirmationDialog::configure(const std::string& newTitle,
                                   const std::string& newMessage,
                                   const std::string& confirmLabel,
                                   const std::string& cancelLabel) {
    title = newTitle;
    message = newMessage;
    confirmButton = Button(0.0f, 0.0f, 120.0f, 52.0f, confirmLabel);
    cancelButton = Button(0.0f, 0.0f, 120.0f, 52.0f, cancelLabel);
}

void ConfirmationDialog::show() {
    visible = true;
    confirmed = false;
    cancelled = false;
}

void ConfirmationDialog::hide() {
    visible = false;
    confirmed = false;
    cancelled = false;
}

bool ConfirmationDialog::isVisible() const {
    return visible;
}

void ConfirmationDialog::update() {
    if (!visible) {
        return;
    }

    updateLayout();
    cancelButton.update();
    confirmButton.update();

    if (cancelButton.isClicked()) {
        cancelled = true;
    }

    if (confirmButton.isClicked()) {
        confirmed = true;
    }
}

void ConfirmationDialog::draw() {
    if (!visible) {
        return;
    }

    updateLayout();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Colors::Black, 0.55f));

    DrawRectangleRounded(panelBounds, 0.08f, 14, Colors::WhiteSmoke);
    DrawRectangleRoundedLinesEx(panelBounds, 0.08f, 14, 3.0f, Colors::DarkBlue);

    int titleSize = std::max(24, static_cast<int>(panelBounds.height * 0.10f));
    int bodySize = std::max(18, static_cast<int>(panelBounds.height * 0.075f));

    int titleWidth = MeasureText(title.c_str(), titleSize);
    int titleX = static_cast<int>(panelBounds.x + (panelBounds.width - titleWidth) * 0.5f);
    int titleY = static_cast<int>(panelBounds.y + panelBounds.height * 0.12f);
    DrawText(title.c_str(), titleX, titleY, titleSize, Colors::Black);

    float messageLeft = panelBounds.x + panelBounds.width * 0.08f;
    float messageTop = panelBounds.y + panelBounds.height * 0.32f;
    float messageWidth = panelBounds.width * 0.84f;
    std::vector<std::string> lines = wrapText(message, bodySize, messageWidth, 4);
    float lineSpacing = bodySize * 1.2f;
    for (size_t i = 0; i < lines.size(); i++) {
        int lineWidth = MeasureText(lines[i].c_str(), bodySize);
        int lineX = static_cast<int>(panelBounds.x + (panelBounds.width - lineWidth) * 0.5f);
        int lineY = static_cast<int>(messageTop + i * lineSpacing);
        DrawText(lines[i].c_str(), lineX, lineY, bodySize, Colors::DarkSlateGray);
    }

    cancelButton.draw();
    confirmButton.draw();
}

bool ConfirmationDialog::consumeConfirmed() {
    const bool value = confirmed;
    confirmed = false;
    return value;
}

bool ConfirmationDialog::consumeCancelled() {
    const bool value = cancelled;
    cancelled = false;
    return value;
}

void ConfirmationDialog::updateLayout() {
    float dialogWidth = std::min(620.0f, GetScreenWidth() * 0.86f);
    float dialogHeight = std::min(360.0f, GetScreenHeight() * 0.62f);
    panelBounds = {
        (GetScreenWidth() - dialogWidth) * 0.5f,
        (GetScreenHeight() - dialogHeight) * 0.5f,
        dialogWidth,
        dialogHeight,
    };

    float buttonWidth = std::max(120.0f, dialogWidth * 0.28f);
    float buttonHeight = std::max(48.0f, dialogHeight * 0.17f);
    float spacing = std::max(14.0f, dialogWidth * 0.05f);
    float buttonsY = panelBounds.y + dialogHeight - buttonHeight - std::max(18.0f, dialogHeight * 0.08f);
    float startX = panelBounds.x + (dialogWidth - (buttonWidth * 2.0f + spacing)) * 0.5f;

    cancelButton.setPosition(startX, buttonsY);
    cancelButton.setSize(buttonWidth, buttonHeight);

    confirmButton.setPosition(startX + buttonWidth + spacing, buttonsY);
    confirmButton.setSize(buttonWidth, buttonHeight);
}
