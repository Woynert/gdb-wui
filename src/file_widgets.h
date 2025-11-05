#ifndef FILE_WIDGETS_H
#define FILE_WIDGETS_H

#include "raylib.h"
#include "raygui.h"
#include "strbuf.h"
#include "strbuf_extra.h"
#include "strview.h"
#include "cwalk.h"
#include "math.h"
#include "portable_utils.h"

inline size_t size_t_max(size_t a, size_t b) { return a > b ? a : b; }
inline size_t size_t_min(size_t a, size_t b) { return a < b ? a : b; }
inline size_t size_t_clamp(size_t min, size_t max, size_t value) {
    return size_t_max(min, size_t_min(max, value)); }
inline int int_max(int a, int b) { return a > b ? a : b; }
inline int int_min(int a, int b) { return a < b ? a : b; }
inline int int_clamp(int min, int max, int value) {
    return int_max(min, int_min(max, value)); }
inline float float_clamp(float min, float max, float value) {
    return fmaxf(min, fminf(max, value)); }

#ifndef MAX_FILEPATH_LENGTH
    #if defined(_WIN32)
        // On Win32, MAX_PATH = 260 (limits.h)
        #define MAX_FILEPATH_LENGTH      256
    #else
        // On Linux, PATH_MAX = 4096 by default (limits.h)
        #define MAX_FILEPATH_LENGTH     4096
    #endif
#endif

static strbuf_space_t(MAX_FILEPATH_LENGTH) FileWidgets_aux_filename_strbuf =
        STRBUF_STATIC_INIT(MAX_FILEPATH_LENGTH);

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

typedef struct FileExplorer {
    union {
        strbuf_space_t(MAX_FILEPATH_LENGTH) _curr_path;
        strbuf_t curr_path;
    };
    File_DynArr files_dyna;
    // gui vars:
    float scroll;
    int scroll_line;
} FileExplorer;

FileExplorer FileExplorer_create(void) {
    FileExplorer file_explorer = { 0 };
    STRBUF_STATIC_INIT2(MAX_FILEPATH_LENGTH, file_explorer._curr_path);
    file_explorer.files_dyna = File_DynArr_create();
    return file_explorer;
}

#define DYN_ARR_TYPE strview_t
#include "./containers/da.h"
#undef DYN_ARR_TYPE

typedef struct FileViewer {
    union {
        strbuf_space_t(MAX_FILEPATH_LENGTH) _file_path;
        strbuf_t file_path;
    };
    strbuf_t *file_data;
    strview_t_DynArr lines;
    // gui vars:
    float scroll;
    int scroll_line;
} FileViewer;


FileViewer FileViewer_create(void) {
    FileViewer viewer = { 0 };
    STRBUF_STATIC_INIT2(MAX_FILEPATH_LENGTH, viewer._file_path);
    viewer.file_data = strbuf_create_empty(0, NULL);
    viewer.lines = strview_t_DynArr_create();
    return viewer;
}


int FileExplorer_list_path(FileExplorer *file_explorer, strview_t path_view) {
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


/// @param panel_rect Bounds of parent component to handle scrolling outside bar
/// @param bar_rect Scroll bar bounds
/// @param[out] out_scroll
/// @param scroll_step For mouse wheel
void ScrollBar_widget(
    Rectangle parent_rect, Rectangle bar_rect,
    float scroll_step, float *out_scroll
) {
    DrawRectangleRec(bar_rect, BLUE);
    Vector2 mouse = GetMousePosition();
    float mouse_wheel = GetMouseWheelMoveV().y;
    float scroll_handle_height = 60;
    Rectangle scrollbar_rect = bar_rect;
    Rectangle scroll_rect = scrollbar_rect;
    scroll_rect.y += scroll_handle_height/2;
    scroll_rect.height -= scroll_handle_height;
    float scroll_percent = ((mouse.y - scroll_rect.y) / scroll_rect.height);

    // mouse wheel
    if (CheckCollisionPointRec(mouse, parent_rect) && mouse_wheel != 0) {
        *out_scroll += (mouse_wheel < 0 ? 1 : -1) * scroll_step;
    }
    // dragging
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)
            && CheckCollisionPointRec(mouse, scrollbar_rect)
    ) {
        *out_scroll = scroll_percent;
    }
    *out_scroll = float_clamp(0, 1, *out_scroll);

    Rectangle scroll_handle = {
        scroll_rect.x,
        0,
        scroll_rect.width,
        scroll_handle_height
    };
    scroll_handle.y = scroll_rect.y -scroll_handle_height/2 + float_clamp(0,
        scroll_rect.height,
        (*out_scroll) * scroll_rect.height
    );

    DrawRectangleRec(scrollbar_rect, GRAY);
    //DrawRectangleRec(scroll_rect, WHITE); // For debugging.
    DrawRectangleRec(scroll_handle, BLUE);
}


/// @param[out] out_selected_file
/// @retval true  file clicked
/// @retval false no file clicked
bool FileExplorer_render(
    FileExplorer *file_explorer, File* out_selected_file, Rectangle panel_rect
) {
    DrawRectangleRec(panel_rect, WHITE);

    bool selected_file = false;
    strbuf_t *aux_file_str = (strbuf_t*)&FileWidgets_aux_filename_strbuf;
    strbuf_assign(&aux_file_str, cstr(""));
    float scrollbar_width = 10;
    int font_height = 10;
    int line_height = font_height + 2;

    strbuf_t *aux_str;
    Rectangle btn_rect = panel_rect;
    btn_rect.height = (float)line_height;
    btn_rect.width -= scrollbar_width + 2;

    int line_count = (int)File_DynArr_get_size(&file_explorer->files_dyna);
    int screen_line_capacity = (int)floorf(panel_rect.height / (float)line_height);
    int from_line = int_clamp(0, line_count, file_explorer->scroll_line);
    int to_line = int_clamp(
            from_line, line_count, from_line + screen_line_capacity);

    for (int i = from_line, k = 0; i < to_line; ++i, ++k) {
        File *file = File_DynArr_get(&file_explorer->files_dyna, (size_t)i);
        if (file == NULL) continue;

        // mouse interaction

        if (CheckCollisionPointRec(GetMousePosition(), btn_rect)) {
            DrawRectangleRec(btn_rect, SKYBLUE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                strbuf_t *selected_path = &file->path;
                strbuf_t *curr_path = &file_explorer->curr_path;

                strbuf_cat(&aux_file_str,
                        strbuf_view(&curr_path),
                        cstr("/"),
                        strbuf_view(&selected_path));
                cwk_path_normalize(aux_file_str->cstr,
                        (char*)aux_file_str->cstr, (size_t)aux_file_str->capacity);
                strbuf_recalculate_size_as_cstr(&aux_file_str);

                selected_file = true;
                *out_selected_file = *file;
                aux_str = &out_selected_file->path;
                strbuf_assign(&aux_str, strbuf_view(&aux_file_str));

                if (!file->is_file) { // open directory
                    printf("Selected file: %s\n", file->path.cstr);
                    FileExplorer_list_path(
                            file_explorer, strbuf_view(&aux_file_str));
                    file_explorer->scroll = 0;
                    file_explorer->scroll_line = 0;
                    break;
                }
            }
        }
        btn_rect.y += (float)line_height;

        // draw text

        aux_str = &file->path;
        strbuf_assign(&aux_file_str, strbuf_view(&aux_str));
        DrawText(aux_file_str->cstr, (int)panel_rect.x, (int)panel_rect.y
                + line_height * (int)k, font_height,
                file->is_file ? BLACK : BLUE
        );
    }

    // scroll bar

    int visible_line_count = line_count
            - (int)((2.f/3.f)*(float)screen_line_capacity); // EOF padding
    Rectangle scrollbar_rect = {
        panel_rect.x + panel_rect.width - scrollbar_width,
        panel_rect.y,
        scrollbar_width,
        panel_rect.height
    };
    float prev_scroll = file_explorer->scroll;
    ScrollBar_widget(panel_rect, scrollbar_rect,
            (1.f/(float)line_count) * 3, &file_explorer->scroll);
    if (prev_scroll != file_explorer->scroll) {
        file_explorer->scroll_line = int_clamp(0, visible_line_count,
            (int)(file_explorer->scroll * (float)(visible_line_count))
        );
    }

    return selected_file;
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
        viewer->scroll_line = 0;
        viewer->scroll = 0;

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


/// Draw scrollable text window.
void FileViewer_render(FileViewer *file_viewer, Rectangle panel_rect) {

    int font_height = 10;
    int screen_line_capacity = (int)floorf(panel_rect.height / (float)font_height);
    int line_count = (int)strview_t_DynArr_get_size(&file_viewer->lines);

    // Draw text

    int from_line = int_clamp(0, line_count, file_viewer->scroll_line);
    int to_line = from_line + screen_line_capacity;
    to_line = int_clamp(from_line, line_count, to_line);

    strbuf_t *aux_file_str = (strbuf_t*)&FileWidgets_aux_filename_strbuf;
    strbuf_assign(&aux_file_str, cstr(""));

    for (int i = from_line, k = 0; i < to_line; ++i, ++k) {
        strview_t *line = strview_t_DynArr_get(&file_viewer->lines, (size_t)i);
        if (line == NULL) continue;

        strbuf_assign(&aux_file_str, *line);
        DrawText(aux_file_str->cstr, (int)panel_rect.x, (int)panel_rect.y
                + font_height * (int)k, font_height, RED);
    }

    // scroll

    int visible_line_count = line_count
            - (int)((2.f/3.f)*(float)screen_line_capacity); // padding at EOF
    float scrollbar_width = 20;
    Rectangle scrollbar_rect = {
        panel_rect.x + panel_rect.width - scrollbar_width,
        panel_rect.y,
        scrollbar_width,
        panel_rect.height
    };

    float prev_scroll = file_viewer->scroll;
    ScrollBar_widget(
        panel_rect,
        scrollbar_rect,
        (1.f/(float)line_count) * 5.f, // scroll 5 lines
        &file_viewer->scroll);

    if (prev_scroll != file_viewer->scroll) {
        file_viewer->scroll_line = int_clamp(0, visible_line_count,
            (int)(file_viewer->scroll * (float)(visible_line_count))
        );
    }
}


#endif // !FILE_WIDGETS_H
