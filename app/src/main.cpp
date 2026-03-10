#include <raylib.h>
#include "game/game.h"

namespace {
void DrawInitialLoadingScreen() {
    ClearBackground(RAYWHITE);

    const char* title = "Color Mandala";
    const int titleFontSize = 52;
    const int titleWidth = MeasureText(title, titleFontSize);
    DrawText(
        title,
        (GetScreenWidth() - titleWidth) / 2,
        (GetScreenHeight() / 2) - 64,
        titleFontSize,
        BLACK
    );

    const char* loadingLabel = "Loading...";
    const int loadingFontSize = 30;
    const int loadingWidth = MeasureText(loadingLabel, loadingFontSize);
    DrawText(
        loadingLabel,
        (GetScreenWidth() - loadingWidth) / 2,
        (GetScreenHeight() / 2) + 8,
        loadingFontSize,
        DARKGRAY
    );
}
}

int main() {
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    const int screenWidth = 0;
    const int screenHeight = 0;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
#else
    const int screenWidth = 1600;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
#endif

    InitWindow(screenWidth, screenHeight, "Color Mandala");
#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WEB)
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(screenWidth, screenHeight);
#endif
    SetTargetFPS(60);

    BeginDrawing();
    DrawInitialLoadingScreen();
    EndDrawing();

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