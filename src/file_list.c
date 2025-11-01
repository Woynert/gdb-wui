#include "stdio.h"
#include "raylib.h"
#include "raygui.h"
#include "strbuf.h"
#include "strview.h"
#include "cwalk.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "sys/wait.h"
#include "errno.h"

#include "assert.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#ifndef MAX_FILEPATH_LENGTH
    #if defined(_WIN32)
        // On Win32, MAX_PATH = 260 (limits.h)
        #define MAX_FILEPATH_LENGTH      256
        // On Linux, PATH_MAX = 4096 by default (limits.h)
    #else
        #define MAX_FILEPATH_LENGTH     4096
    #endif
#endif



typedef struct File {
    char path[MAX_FILEPATH_LENGTH];
    bool is_file;
} File;


#define DYN_ARR_TYPE File
#include "./containers/da.h"
#undef DYN_ARR_TYPE

#define DYN_ARR_TYPE size_t
#include "./containers/da.h"
#undef DYN_ARR_TYPE


typedef struct {
    char curr_path[MAX_FILEPATH_LENGTH];
    File_DynArr files_dyna;
} FileExplorer;


typedef struct {
    char file_path[MAX_FILEPATH_LENGTH];
    char *file_data;
    size_t file_data_size;
    size_t_DynArr newlines; // marks where are the newlines
} FileViewer;


FileViewer FileViewer_create(void) {
    FileViewer viewer = { 0 };
    viewer.newlines = size_t_DynArr_create();
    viewer.file_data_size = 1;
    viewer.file_data = (char*)calloc(1, viewer.file_data_size);
    return viewer;
}


/// @returns error
int FileViewer_load_file(FileViewer *viewer, const char* path) {
    if (!FileExists(path) || !IsPathFile(path)) {
        return -1;
    }

    char *file_content = LoadFileText(path);
    size_t content_size = strlen(file_content);

    memset(viewer->file_path, 0, sizeof(viewer->file_path));
    strcpy(viewer->file_path, path);

    if (content_size > viewer->file_data_size) {
        viewer->file_data_size = content_size+1;
        viewer->file_data = (char*)realloc(viewer->file_data, viewer->file_data_size);
    }

    strcpy(viewer->file_data, file_content);

    // find newlines
    size_t_DynArr_clear_preserving_capacity(&viewer->newlines);
    for (size_t i = 0; i < content_size; ++i) {
        if (file_content[i] == '\n') {
            size_t_DynArr_insert(&viewer->newlines, i);
        }
    }

    free(file_content);
    printf("DEBUG: Succesfully loaded file\n");
    return OK;
}


int intmin(int a, int b) { return a > b ? b : a; }
uint uintmin(uint a, uint b) { return a > b ? b : a; }
size_t size_tmin(size_t a, size_t b) { return a > b ? b : a; }


void list_files(FileExplorer *file_explorer, const char* path) {
    file_explorer->files_dyna = File_DynArr_create();
    strcpy(file_explorer->curr_path, path);
    /*file_explorer->curr_path[]*/
    printf("%s\n", file_explorer->curr_path);

    File_DynArr *files_dyna = &file_explorer->files_dyna;
    FilePathList file_list = LoadDirectoryFiles(path);

    {
        // Add previous
        File file = { 0 };
        file.is_file = false;
        strcat(file.path, path);
        strcat(file.path, "/..");
        File_DynArr_insert(files_dyna, file);
    }

    for (uint i = 0; i < file_list.count; i++) {
        if (IsPathFile(file_list.paths[i])) continue;
        File file = { 0 };
        strncpy(file.path, file_list.paths[i],
                uintmin(sizeof(file.path), (uint)strlen(file_list.paths[i])));
        file.path[sizeof(file.path)-1] = 0;
        file.is_file = false;
        File_DynArr_insert(files_dyna, file);
    }
    for (uint i = 0; i < file_list.count; i++) {
        if (!IsPathFile(file_list.paths[i])) continue;
        File file = { 0 };
        strncpy(file.path, file_list.paths[i],
                uintmin(sizeof(file.path), (uint)strlen(file_list.paths[i])));
        file.path[sizeof(file.path)-1] = 0;
        file.is_file = true;
        File_DynArr_insert(files_dyna, file);
    }

    File_DynArrIterator it = { 0 };
    while (File_DynArr_iterator_get_next(files_dyna, &it) == OK) {
        printf("%s %s\n",
                it.item->path,
                it.item->is_file ? "FILE" : "DIR");
    }
}

void strbuf_recalculate_size_as_cstr(strbuf_t** buf_ptr) {
    strbuf_t* buf = *buf_ptr;
    buf->size = (int)strlen(buf->cstr);
}


int main (void) {

    printf("Hello there\n");

    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "File list demo");
    SetTargetFPS(60);

    FileExplorer file_explorer = { 0 };
    file_explorer.files_dyna = File_DynArr_create();
    strcpy(file_explorer.curr_path, GetWorkingDirectory());
    assert(strlen(GetWorkingDirectory())-1 <= sizeof(file_explorer.curr_path));

    FileViewer file_viewer = FileViewer_create();
    Rectangle panel_view = { 0 };
    Vector2 panel_scroll = { 0 };

    printf("%s\n", file_explorer.curr_path);

    list_files(&file_explorer, GetWorkingDirectory());

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        BeginDrawing();

        ClearBackground(BLACK);
        DrawFPS(0,0);

        char str_file_name[MAX_FILEPATH_LENGTH] = { 0 };
        const float BTN_V_PAD = 1;
        Rectangle btn_rect = { 20, 40, 100, 15 };
        uint curr_path_str_len = (uint)strlen(file_explorer.curr_path) +1;

        File_DynArrIterator it = { 0 };
        while (File_DynArr_iterator_get_next(
                    &file_explorer.files_dyna, &it) == OK)
        {
            File *file = it.item;

            // name formatting
            uint str_len = (uint)strlen(file->path) -curr_path_str_len;
            strncpy(str_file_name,
                    file->path +curr_path_str_len,
                    str_len
            );
            str_file_name[
                uintmin(
                    sizeof(str_file_name)-1,
                    str_len
                )
            ] = 0;
            if (!file->is_file
                && strlen(str_file_name)+1 < sizeof(str_file_name)
            ){
                    strcat(str_file_name, "/");
            }

            // actual UI

            if (GuiLabelButton(btn_rect, str_file_name)) {
                if (!file->is_file) {
                    printf("Selected file: %s\n", file->path);
                    list_files(&file_explorer, file->path);
                    break;
                }
                else {
                    FileViewer_load_file(&file_viewer, file->path);
                }
            }
            btn_rect.y += btn_rect.height + BTN_V_PAD;
        }
        
        // TODO: WRAP ALL OF THESE INTO FUNCTIONS

        // Draw scrollable text window

        Rectangle panel_rect = { 200, 50, 300, 400 };
        Rectangle panel_content_rect = panel_rect;
        panel_content_rect.height = 600;
        panel_content_rect.width -= 40;
        panel_content_rect.height -= 40;
        Vector2 scroll_maximum;

        GuiScrollPanel(panel_rect, "Hello there", panel_content_rect,
                &panel_scroll, &panel_view, &scroll_maximum);

        // Draw text

        float scroll_percent =
                        fabsf(panel_scroll.y / scroll_maximum.y);
        /*float scroll_percent = panel_scroll.y / scroll_maximum.y;*/
        int font_height = 10;
        size_t line_count = size_t_DynArr_get_size(&file_viewer.newlines);
        // remove visible lines
        line_count -= (size_t)floorf(panel_view.height / (float)font_height);

        size_t from_line = (size_t)floorf(scroll_percent * (float)line_count);
        size_t to_line = from_line + (size_t)(panel_view.height / (float)font_height);

        /*printf("%zu %zu\n", from_line, to_line);*/

        if (file_viewer.file_data_size > 0) {
            char text_line[255] = { 0 };
            for (size_t i = from_line, k = 0; i < to_line; ++i, ++k) {
                if (i+1 >= size_t_DynArr_get_size(&file_viewer.newlines)) {
                    break;
                }

                size_t from_char = *size_t_DynArr_get(&file_viewer.newlines, i)+1;
                size_t to_char = *size_t_DynArr_get(&file_viewer.newlines, i+1);

                /*printf("i%zu %zu\n", k, to_char - from_char );*/

                size_t size = size_tmin(sizeof(text_line)-1, to_char - from_char);
                strncpy(text_line, file_viewer.file_data + from_char, size);
                text_line[size] = 0;

                DrawText(text_line, (int)panel_view.x, (int)panel_view.y
                        + font_height * (int)k, font_height, RED);
            }
        }

        EndDrawing();
    }

    return 0;
}
