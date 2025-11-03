#include "stdio.h"
#include "raylib.h"
#include "raygui.h"
#include "strbuf.h"
#include "strbuf_extra.h"
#include "strview.h"
#include "cwalk.h"
#include "portable_utils.h"

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


inline size_t size_t_max(size_t a, size_t b) { return a > b ? a : b; }
inline size_t size_t_min(size_t a, size_t b) { return a < b ? a : b; }
inline size_t size_t_clamp(size_t min, size_t max, size_t value) {
    return size_t_max(min, size_t_min(max, value));
}
inline int int_max(int a, int b) { return a > b ? a : b; }
inline int int_min(int a, int b) { return a < b ? a : b; }
inline int int_clamp(int min, int max, int value) {
    return int_max(min, int_min(max, value));
}

typedef struct File {
    bool is_file;
    union {
        strbuf_space_t(MAX_FILEPATH_LENGTH) _path;
        strbuf_t path;
    };
} File;

File File_create(void) {
    File file = { 0 };
    STRBUF_STATIC_INIT2(MAX_FILEPATH_LENGTH, file._path);
    return file;
}

#define DYN_ARR_TYPE File
#include "./containers/da.h"
#undef DYN_ARR_TYPE

#define DYN_ARR_TYPE size_t
#include "./containers/da.h"
#undef DYN_ARR_TYPE

#define DYN_ARR_TYPE strview_t
#include "./containers/da.h"
#undef DYN_ARR_TYPE

typedef struct {
    union {
        strbuf_space_t(MAX_FILEPATH_LENGTH) _curr_path;
        strbuf_t curr_path;
    };
    File_DynArr files_dyna;
} FileExplorer;

FileExplorer FileExplorer_create(void) {
    FileExplorer file_explorer = { 0 };
    STRBUF_STATIC_INIT2(MAX_FILEPATH_LENGTH, file_explorer._curr_path);
    file_explorer.files_dyna = File_DynArr_create();
    return file_explorer;
}

typedef struct {
    union {
        strbuf_space_t(MAX_FILEPATH_LENGTH) _file_path;
        strbuf_t file_path;
    };
    strbuf_t *file_data;
    strview_t_DynArr lines;
} FileViewer;

FileViewer FileViewer_create(void) {
    FileViewer viewer = { 0 };
    STRBUF_STATIC_INIT2(MAX_FILEPATH_LENGTH, viewer._file_path);
    viewer.file_data = strbuf_create_empty(0, NULL);
    viewer.lines = strview_t_DynArr_create();
    return viewer;
}

/// @returns error
int FileViewer_load_file(FileViewer *viewer, strview_t path_view) {
    int return_error = 0;
    strbuf_t *path = NULL;
    strbuf_t *file_content = NULL;

    do {
        path = strbuf_create_init(path_view, NULL);
        if (!FileExists(path->cstr) || !IsPathFile(path->cstr)) {
            return_error = -1;
            break;
        }

        // Must free constents from RayLib
        {
            file_content = strbuf_create(cstr(LoadFileText(path->cstr)), NULL);
            if (viewer->file_data == 0) {
                return_error = -2;
                break;
            }
            strbuf_assign(&viewer->file_data, strbuf_view(&file_content));
        }

        {
            strbuf_t *tmp = &viewer->file_path;
            strbuf_assign(&tmp, path_view);
        }

        // find newlines

        strview_t line_reader = strbuf_view(&viewer->file_data);
        strview_t line = { 0 };
        strview_t_DynArr_clear_preserving_capacity(&viewer->lines);

        do {
            line = strview_split_first_delim(&line_reader, "\n", true);
            strview_t_DynArr_insert(&viewer->lines, line);
            printf("(%d) %"PRIstr"\n", line.size, PRIstrarg(line));
        }
        while (line_reader.size > 0 || !strview_is_valid(line));

        printf("DEBUG: Succesfully loaded file\n");
        return_error = OK;
    } while (0);

    strbuf_destroy(&path);
    strbuf_destroy(&file_content);
    return return_error;
}


int intmin(int a, int b) { return a > b ? b : a; }
uint uintmin(uint a, uint b) { return a > b ? b : a; }
size_t size_tmin(size_t a, size_t b) { return a > b ? b : a; }


int list_files(FileExplorer *file_explorer, strview_t path_view) {
    strbuf_t *path = strbuf_create_init(path_view, NULL);
    if (!FileExists(path->cstr) || IsPathFile(path->cstr)) {
        return -1;
    }

    File_DynArr_clear_preserving_capacity(&file_explorer->files_dyna);

    {
        strbuf_t *file_explorer_curr_path = &file_explorer->curr_path;
        strbuf_assign(&file_explorer_curr_path, path_view);
    }
    printf("%s\n", file_explorer->curr_path.cstr);

    File_DynArr *files_dyna = &file_explorer->files_dyna;
    FilePathList file_list = LoadDirectoryFiles(path->cstr);

    {
        // Add previous
        File file = File_create();
        file.is_file = false;
        {
            strbuf_t *file_path = &file.path;
            strbuf_assign(&file_path, cstr(".."));
        }
        File_DynArr_insert(files_dyna, file);
    }

    for (uint i = 0; i < file_list.count; i++) { // directories
        if (IsPathFile(file_list.paths[i])) continue;
        strview_t basename;
        size_t length;
        cwk_path_get_basename(file_list.paths[i], &basename.data, &length);
        basename.size = (int)length;

        File file = File_create();
        file.is_file = false;
        {
            strbuf_t *file_path = &file.path;
            strbuf_assign(&file_path, basename);
        }
        File_DynArr_insert(files_dyna, file);
    }
    for (uint i = 0; i < file_list.count; i++) { // files
        if (!IsPathFile(file_list.paths[i])) continue;
        strview_t basename;
        size_t length;
        cwk_path_get_basename(file_list.paths[i], &basename.data, &length);
        basename.size = (int)length;

        File file = File_create();
        file.is_file = true;
        {
            strbuf_t *file_path = &file.path;
            strbuf_assign(&file_path, basename);
        }
        File_DynArr_insert(files_dyna, file);
    }

    File_DynArrIterator it = { 0 };
    while (File_DynArr_iterator_get_next(files_dyna, &it) == OK) {
        printf("%s %s\n",
                it.item->path.cstr,
                it.item->is_file ? "FILE" : "DIR");
    }

    UnloadDirectoryFiles(file_list);
    return OK;
}

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

    Rectangle panel_view = { 0 };
    Vector2 panel_scroll = { 0 };

    strbuf_t *aux_file_str = strbuf_create_empty(MAX_FILEPATH_LENGTH, NULL);

    printf("%s\n", file_explorer.curr_path.cstr);

    int error = list_files(&file_explorer, cstr(file_explorer.curr_path.cstr));
    printf("%d\n", error);
    ASSERT(error == OK);

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        BeginDrawing();

        ClearBackground(BLACK);
        DrawFPS(0,0);

        const float BTN_V_PAD = 1;
        Rectangle btn_rect = { 20, 40, 100, 15 };

        File_DynArrIterator it = { 0 };
        while (File_DynArr_iterator_get_next(
                    &file_explorer.files_dyna, &it) == OK)
        {
            File *file = it.item;

            // name formatting
            {
                strbuf_t *tmp = &file->path;
                strbuf_assign(&aux_file_str, strbuf_view(&tmp));
            }

            if (!file->is_file) {
                strbuf_append(&aux_file_str, "/");
            }

            // actual UI

            if (GuiLabelButton(btn_rect, aux_file_str->cstr)) {

                strbuf_t *selected_path = &file->path;
                strbuf_t *curr_path = &file_explorer.curr_path;
                strbuf_cat(&aux_file_str, strbuf_view(&curr_path), cstr("/"), strbuf_view(&selected_path));

                cwk_path_normalize(aux_file_str->cstr, (char*)aux_file_str->cstr, (size_t)aux_file_str->capacity);
                strbuf_recalculate_size_as_cstr(&aux_file_str);

                if (!file->is_file) { // open directory
                    printf("Selected file: %s\n", file->path.cstr);
                    list_files(&file_explorer, strbuf_view(&aux_file_str));
                    break;
                }
                else { // preview file
                    FileViewer_load_file(&file_viewer, strbuf_view(&aux_file_str));
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
        int font_height = 10;
        int line_count = (int)strview_t_DynArr_get_size(&file_viewer.lines);

        // remove visible lines
        int screen_line_capacity = (int)floorf(panel_view.height / (float)font_height);

        int from_line = (int)floorf(scroll_percent * (float)(line_count - screen_line_capacity));
        from_line = int_clamp(0, line_count, from_line);

        int to_line = from_line + screen_line_capacity;
        to_line = int_clamp(from_line, line_count, to_line);

        /*printf("from %d to %d | %d\n", from_line, to_line, line_count);*/

        for (int i = from_line, k = 0; i < to_line; ++i, ++k) {
            strview_t *line = strview_t_DynArr_get(&file_viewer.lines, (size_t)i);
            if (line == NULL) continue;

            strbuf_assign(&aux_file_str, *line);
            DrawText(aux_file_str->cstr, (int)panel_view.x, (int)panel_view.y
                    + font_height * (int)k, font_height, RED);
        }

        EndDrawing();
    }

    return 0;
}
