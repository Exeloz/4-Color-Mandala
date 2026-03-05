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
        : message("Are you sure?"), popup() {
        popup.configure("Confirm", "Confirm", "Cancel");
        popup.setSizeRatios(0.86f, 0.62f);
}

void ConfirmationDialog::configure(const std::string& newTitle,
                                   const std::string& newMessage,
                                   const std::string& confirmLabel,
                                   const std::string& cancelLabel) {
    message = newMessage;
    popup.configure(newTitle, confirmLabel, cancelLabel);
}

void ConfirmationDialog::show() {
    popup.show();
}

void ConfirmationDialog::hide() {
    popup.hide();
}

bool ConfirmationDialog::isVisible() const {
    return popup.isVisible();
}

void ConfirmationDialog::update() {
    popup.update();
}

void ConfirmationDialog::draw() {
    if (!popup.isVisible()) {
        return;
    }

    popup.drawShell();

    Rectangle panelBounds = popup.getBounds();

    int titleSize = std::max(24, static_cast<int>(panelBounds.height * 0.10f));
    int bodySize = std::max(18, static_cast<int>(panelBounds.height * 0.075f));

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

    popup.drawButtons();
}

bool ConfirmationDialog::consumeConfirmed() {
    return popup.consumeConfirmed();
}

bool ConfirmationDialog::consumeCancelled() {
    return popup.consumeCancelled();
}
