#include "raylib.h"

#include <format>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    constexpr int screenWidth = 800;
    constexpr int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - delta time");

    SetTargetFPS(120);

    auto ballPosition = Vector2 {screenWidth / 2, screenHeight / 2};

        while (not WindowShouldClose()) {
            BeginDrawing();

                if (IsKeyDown(KEY_A)) {
                    ballPosition.x -= GetFrameTime() * 0.6 + 2.0f;
                }

                if (IsKeyDown(KEY_D)) {
                    ballPosition.x += GetFrameTime() * 0.6 + 2.0f;
                }

                if (IsKeyDown(KEY_W)) {
                    ballPosition.y -= GetFrameTime() * 0.6 + 2.0f;
                }

                if (IsKeyDown(KEY_S)) {
                    ballPosition.y += GetFrameTime() * 0.6 + 2.0f;
                }

            const auto fpsStr = std::format("Current Fps {}", GetFPS());
            DrawText(fpsStr.data(), 25, 25, 16, GRAY);

            DrawCircleV(ballPosition, 50, BROWN);

            ClearBackground(RAYWHITE);
            EndDrawing();
        }

    CloseWindow();

    return 0;
}