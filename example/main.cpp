#include "raylib.h"

#include <cmath>
#include <format>
#include <ranges>
#include <vector>

auto calcCircleDistance(const Vector2& a, const Vector2& b) -> double {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main() {
    // Initialization
    //--------------------------------------------------------------------------------------
    constexpr int screenWidth = 800;
    constexpr int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - delta time");

    SetTargetFPS(120);

    std::vector<Vector2> ballPositions;

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

            const auto mousePosition = GetMousePosition();

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ballPositions.push_back({mousePosition.x, mousePosition.y});
                }

                // 判断鼠标右键按下的位置是否在一个圆内，如果存在则删除此坐标
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    const auto iter = std::ranges::find_if(
                        ballPositions.begin(),
                        ballPositions.end(),
                        [&](const Vector2& vec) {
                            return calcCircleDistance(vec, mousePosition) < 15;
                        }
                    );
                        if (iter != ballPositions.end()) {
                            ballPositions.erase(iter);
                        }
                }

                for (const auto& pos: ballPositions) {
                    DrawCircleV(pos, 15, PURPLE);
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