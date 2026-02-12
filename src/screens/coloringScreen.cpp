#include "coloringScreen.h"
#include "../ui/colors.h"
#include "../ui/input.h"

ColoringScreen::ColoringScreen(std::shared_ptr<Mandala> mandala)
    : mandala(mandala), colorPalette(), colorButtons(), backButton(20, 20, 100, 50, "BACK"),
      gameWon(false), returnRequested(false) {
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        float x = 50 + i * 130;
        float y = 550;
        colorButtons.emplace_back(x, y, 100, 40, "");
    }
}

void ColoringScreen::update(float deltaTime) {
    backButton.update();
    if (backButton.isClicked()) {
        returnRequested = true;
        return;
    }

    for (int i = 0; i < static_cast<int>(colorButtons.size()); i++) {
        colorButtons[i].update();
        if (colorButtons[i].isClicked()) {
            colorPalette.setSelectedColorIndex(i);
        }
    }

    handleColorSelection();

    if (mandala->isValidColoring()) {
        gameWon = true;
    }
}

void ColoringScreen::handleColorSelection() {
    if (Input::IsPointerPressed()) {
        Vector2 pointerPos = Input::GetPointerPosition();
        Region* region = mandala->getRegionAtPoint(pointerPos);
        if (region != nullptr) {
            region->setColor(colorPalette.getSelectedColorIndex());
        }
    }
}

void ColoringScreen::draw() {
    ClearBackground(Colors::Gainsboro);
    
    DrawText(mandala->getName().c_str(), 150, 20, 30, Colors::Black);
    
    mandala->draw(colorPalette.getColors());

    drawColorPalette();
    backButton.draw();
}

void ColoringScreen::drawColorPalette() {
    DrawText("Colors:", 50, 530, 15, Colors::Black);
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        float x = 50 + i * 130;
        float y = 550;
        Color color = colorPalette.getColor(i);
        
        DrawRectangle(x, y, 100, 40, color);
        
        if (i == colorPalette.getSelectedColorIndex()) {
            DrawRectangleLinesEx({x, y, 100, 40}, 4, Colors::Black);
        } else {
            DrawRectangleLinesEx({x, y, 100, 40}, 1, Colors::Gray);
        }
    }
}

bool ColoringScreen::isGameWon() const {
    return gameWon;
}

bool ColoringScreen::shouldReturnToSelection() const {
    return returnRequested;
}
