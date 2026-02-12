#include <raylib.h>
#include "game/game.h"

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Color Mandala");
    SetTargetFPS(60);

    Game game;
    game.initialize();

    while (!game.shouldClose()) {
        float deltaTime = GetFrameTime();
        
        game.update(deltaTime);

        BeginDrawing();
        game.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}