#include "stdio.h"
#include "raylib.h"
#include "raygui.h"

int main (void) {
    printf("Hello there\n");

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
    SetTargetFPS(30);

    while (!WindowShouldClose())
    {
        BeginDrawing();

            ClearBackground(RAYWHITE);
            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

            Rectangle btn_ok = {
                10, 10, 100, 50
            };
            if (GuiButton(btn_ok, "Continue")) {
                printf("You pressed me!\n");
            }

        EndDrawing();
    }
    CloseWindow();
}
