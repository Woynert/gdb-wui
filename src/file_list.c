#include "assert.h"
#include "file_widgets.h"
#include "portable_utils.h"
#include "raygui.h"
#include "raylib.h"
#include "stdio.h"
#include "strbuf.h"
#include "strview.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>


int main (void) {

    printf("Hello there\n");

    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "File list demo");
    SetTargetFPS(60);

    FileExplorer file_explorer = FileExplorer_create();
    {
        strbuf_t *tmp = &file_explorer.curr_path;
        strbuf_assign(&tmp, cstr(GetWorkingDirectory()));
    }

    FileViewer file_viewer = FileViewer_create();


    printf("%s\n", file_explorer.curr_path.cstr);

    int error = FileExplorer_list_path(&file_explorer, cstr(file_explorer.curr_path.cstr));
    printf("%d\n", error);
    ASSERT(error == OK);
    File selected_file = { 0 };

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(0,0);

        Rectangle file_explorer_rect = { 20, 40, 150, 200 };
        bool file_clicked = FileExplorer_render(
                &file_explorer, &selected_file, file_explorer_rect);

        if (file_clicked && selected_file.is_file) {
            printf("Selected file: %s (file? %d)\n",
                    selected_file.path.cstr, selected_file.is_file);
            {
                strbuf_t *tmp = &selected_file.path;
                FileViewer_load_file(&file_viewer, strbuf_view(&tmp));
            }
        }

        Rectangle file_viewer_rect = { 200, 50, 300, 400 };
        DrawRectangleRec(file_viewer_rect, WHITE);
        FileViewer_render(&file_viewer, file_viewer_rect);

        EndDrawing();
    }

    return 0;
}
