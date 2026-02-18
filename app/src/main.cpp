#include <raylib.h>
#include "game/game.h"

int main() {
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    const int screenWidth = 0;
    const int screenHeight = 0;
#else
    const int screenWidth = 860;
    const int screenHeight = 420;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif

    InitWindow(screenWidth, screenHeight, "Color Mandala");
#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WEB)
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(560, 320);
#endif
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