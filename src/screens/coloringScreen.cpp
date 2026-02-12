#include "coloringScreen.h"

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
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();
        Region* region = mandala->getRegionAtPoint(mousePos);
        if (region != nullptr) {
            region->setColor(colorPalette.getSelectedColorIndex());
        }
    }
}

void ColoringScreen::draw() {
    ClearBackground({240, 240, 240, 255});
    
    DrawText(mandala->getName().c_str(), 150, 20, 30, {0, 0, 0, 255});
    
    mandala->draw(std::vector<Color>{
        {255, 0, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 0, 255},
        {0, 255, 0, 255},
        {255, 255, 255, 255}
    });

    drawColorPalette();
    backButton.draw();
}

void ColoringScreen::drawColorPalette() {
    DrawText("Colors:", 50, 530, 15, {0, 0, 0, 255});
    
    for (int i = 0; i < colorPalette.getColorCount(); i++) {
        float x = 50 + i * 130;
        float y = 550;
        Color color = colorPalette.getColor(i);
        
        DrawRectangle(x, y, 100, 40, color);
        
        if (i == colorPalette.getSelectedColorIndex()) {
            DrawRectangleLinesEx({x, y, 100, 40}, 4, {0, 0, 0, 255});
        } else {
            DrawRectangleLinesEx({x, y, 100, 40}, 1, {100, 100, 100, 255});
        }
    }
}

bool ColoringScreen::isGameWon() const {
    return gameWon;
}

bool ColoringScreen::shouldReturnToSelection() const {
    return returnRequested;
}
