#ifndef GUI

#include "stdio.h"
#include "math.h"

#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "raylib_extra.h"
#include "strbuf.h"
#include "strnum.h"
#include "strview.h"
#include "strbuf_extra.h"
#include "portable_utils.h"

#include "wui_state.h"




/* Notes:
 * Since strbuf empties the string if the space is insufficient (for static
 * strings) then we need to use a large enough buffer as a temporary container
 * and then copy just the size we need. Ideally we would use a strbuf_assign_n
 * will would allow to copy only the first n characters.
 */
strbuf_space_t(4096) _gui_str = STRBUF_STATIC_INIT(4096);
strbuf_t *gui_str = (strbuf_t*)(&_gui_str);

enum POPUP {
    POPUP_NONE,
    POPUP_CONTEXT_MENU,
    POPUP_TEXT_EDITOR,
};

#define GUI_OPTIONS_STRING_LEN 1024
#define GUI_POPUP_PAD 3

/* Text Editor */

#ifndef DYN_ARR_TYPE_UINT
#define DYN_ARR_TYPE_UINT
#define DYN_ARR_TYPE uint
#undef  DYN_ARR_PREFIX
#include "containers/da.h"
#undef  DYN_ARR_TYPE
#endif

typedef struct TextEdit_Change {
    bool is_insert;  // false == deletion
    int start;
    strbuf_t *buffer;
} TextEdit_Change;

#define WRING__TYPE TextEdit_Change
#define WRING__NAMESPACE TextEditRing
#include "containers/wring.h"
#undef WRING__TYPE
#undef WRING__NAMESPACE

#define TEXTEDIT_LOG_SIZE 256

typedef struct TextEdit_Log {
    TextEditRing ring;
    uint cursor;
} TextEdit_Log;

typedef struct TextEdit {
    bool enabled;
    int cursor;
    //bool wrap;
    int scroll;
    bool is_selecting;
    int selection_origin;
    int selection_line;
    strbuf_t *buffer; // grows as needed (it's cleared on close (manually))
    uint_DynArr linebreaks;
    int line_amount;
    TextEdit_Log editlog;
    /* cached */
    int cursor_line;
} TextEdit;

void GUI_TextEdit_init(TextEdit *textedit) {
    textedit->buffer = strbuf_create(0, NULL);
    textedit->linebreaks = uint_DynArr_create();

    textedit->editlog.ring = TextEditRing_create(TEXTEDIT_LOG_SIZE);
    for (int i = 0; i < TEXTEDIT_LOG_SIZE; ++i) {
        textedit->editlog.ring.buffer[i].buffer = strbuf_create(0, NULL);
    }
}

struct {
    Font font;
    float font_size;
    float font_width;
    float font_spacing;

    RenderTexture2D aux_texture;
    RenderTexture2D final_texture;

    /* TODO:
    int curr_focus=
       FOCUS_AVAILABLE,
       FOCUS_BUSY_POPUP
       FOCUS_BUSY_DRAGGING (For resizing panes)
    */
    enum POPUP curr_popup;

    struct {
        int selected_option;
        int option_amount;
        Vector2 origin;
        union {
            Vector2 size;
            struct {
                float width;
                float height;
            };
        };
        union {
            strbuf_space_t(GUI_OPTIONS_STRING_LEN) _options;
            strbuf_t options;
        };
    } context_menu;

    TextEdit textedit;
} GUI;

void GUI_init_global_context(void) {
    STRBUF_STATIC_INIT2(GUI_OPTIONS_STRING_LEN, GUI.context_menu._options);

    // TODO: what about multiscreen setups?
    GUI.aux_texture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    GUI.final_texture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    GUI_TextEdit_init(&GUI.textedit);

    //LoadFont
    GUI.font_spacing = 1;
    GUI.font_size = 15;
    GUI.font = LoadFontEx("./IosevkaFixed-Regular.ttf", (int)GUI.font_size, 0, 250);
    //GUI.font = LoadFontEx("./assets/RobotoMono-Regular.ttf", (int)GUI.font_size, 0, 250);
    //SetTextureFilter(draw_ctx.font.texture, TEXTURE_FILTER_POINT);

    int w1 = (int)MeasureTextEx(GUI.font, "W",
            (float)GUI.font_size, GUI.font_spacing).x;
    int w2 = (int)MeasureTextEx(GUI.font, "@",
            (float)GUI.font_size, GUI.font_spacing).x;
    int w3 = (int)MeasureTextEx(GUI.font, "_",
            (float)GUI.font_size, GUI.font_spacing).x;
    GUI.font_width = (float)int_max(w1, int_max(w2, w3));
}

void GUI_open_context_menu(strview_t options_str) {
    if (options_str.size <= 0) return;
    GUI.curr_popup = POPUP_CONTEXT_MENU;
    GUI.context_menu.origin = GetMousePosition();

    // Get count.

    GUI.context_menu.option_amount = 0;
    for (int i = 0; i < options_str.size; ++i) {
        if (options_str.data[i] == '\n')
            { ++GUI.context_menu.option_amount; }
    }

    strbuf_t *tmp = &GUI.context_menu.options;
    strview_t all_lines = options_str;
    strview_t line;
    GUI.context_menu.width = 100;

    // Measure width.

    bool must_break = false;
    while(1) {
        if (all_lines.size == 0 || must_break) { break; }
        line = strview_split_line(&all_lines, NULL);
        if (!strview_is_valid(line)) { line = all_lines; must_break = true; }
        if (!strview_is_valid(line)) { continue; }

        strbuf_assign(&tmp, line);
        GUI.context_menu.width = fmaxf(
            GUI.context_menu.width,
            (float)(int)MeasureTextEx(
                GUI.font, tmp->cstr, GUI.font_size, GUI.font_spacing
            ).x
        );
    }
    GUI.context_menu.width += 20;
    GUI.context_menu.height =
        (float)GUI.context_menu.option_amount * (GUI.font_size + GUI_POPUP_PAD);

    // Set origin so that it doesn't go out of the window.

    Vector2 delta =
        Vector2Subtract(
            (Vector2) { (float)GetScreenWidth(), (float)GetScreenHeight() },
            Vector2Add(GUI.context_menu.origin, GUI.context_menu.size)
        );
    if (delta.x < 0) { GUI.context_menu.origin.x += delta.x; }
    if (delta.y < 0) { GUI.context_menu.origin.y += delta.y; }

    strbuf_assign(&tmp, options_str);
}

void GUI_close_popup(int option_selected) {
    GUI.curr_popup = POPUP_NONE;
}


//typedef struct GuiBreakpointView {
    //int i;
//} GuiBreakpointView;


void GuiBreakpointView_draw(WuiState *state, Rect2 view_rect) {

    BeginTextureMode(GUI.aux_texture);
    DrawRectangleRec(view_rect.rect, GRAY);

    float line_height = GUI.font_size + 2;
    for (size_t i = 0; i < state->breakpoints.size; ++i) {
        WuiBreakpoint *breakpoint = &state->breakpoints.items[i];

        strbuf_printf(&gui_str, "%d%s %s %s",
            breakpoint->id,
            breakpoint->enabled ? "" : " OFF",
            breakpoint->location.cstr,
            breakpoint->file.cstr);

        DrawTextEx(GUI.font, gui_str->cstr,
            (Vector2){ view_rect.x, view_rect.y + (float)i * line_height},
            GUI.font_size, GUI.font_spacing, BLACK);
    }

    DrawRectangleLinesEx(view_rect.rect, 1, BLACK);
    EndTextureMode();

    // Open context menu.

    if (CheckCollisionPointRec(GetMousePosition(), view_rect.rect)
        && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
    ) {
        printf("Clicked!\n");
        GUI_open_context_menu(cstr(
            "Option 1\n"
            "Option 2\n"
            "Option 3: This will be a long description\n"
            "Option 4\n"
            "Option 5\n"
        ));
    }
}

void GUI_draw_popups(void) {
    if (GUI.curr_popup == POPUP_NONE) { return; }
    else if (GUI.curr_popup == POPUP_CONTEXT_MENU) {
        Vector2 mouse = GetMousePosition();
        strview_t all_lines = strview_of_buf(&GUI.context_menu.options);
        strview_t line = { 0 };
        int line_height = (int)GUI.font_size + GUI_POPUP_PAD;
        Rect2 rect = {{
            GUI.context_menu.origin.x,
            GUI.context_menu.origin.y,
            (float)GUI.context_menu.width,
            (float)GUI.context_menu.option_amount * (float)line_height
        }};
        Rect2 option_rect = rect;
        option_rect.height = (float)line_height;
        bool hover = false;
        bool must_break = false;
        int hover_option_id = -1;

        DrawRectangleRec(rect.rect, DARKGRAY);

        for (int i = 0;; ++i) {
            if (all_lines.size == 0 || must_break) { break; }
            line = strview_split_line(&all_lines, NULL);
            if (!strview_is_valid(line)) { line = all_lines; must_break = true; }
            if (!strview_is_valid(line)) { continue; }

            option_rect.y = rect.y + (float)i * (float)line_height;
            hover = CheckCollisionPointRec(mouse, option_rect.rect);

            //printf("-> %"PRIstr"\n", PRIstrarg(line));

            if (hover) {
                DrawRectangleRec(option_rect.rect, BLUE);
                hover_option_id = i;
            }

            DrawTextEx_strview(GUI.font, line,
                (Vector2) { rect.x + 2, rect.y + (float)i * (float)line_height },
                GUI.font_size, GUI.font_spacing, 2, BLACK);
        }

        DrawRectangleLinesEx(rect.rect, 1, BLACK);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            printf("Pressed option %d\n", hover_option_id);
            GUI_close_popup(hover_option_id);
        }
    }
}

// FIXME: This startindex doesn't work because we are not preserving previous
//        lines.
void GUI_TextEdit__update_linebreaks(TextEdit *textedit, int startindex) {
    strview_t buffer = strview_of_buf(textedit->buffer);
    if (startindex >= buffer.size) { startindex = 0; };
    uint_DynArr_clear_preserving_capacity(&textedit->linebreaks);

    uint_DynArr_insert(&textedit->linebreaks, 0);
    for (int i = startindex; i < textedit->buffer->size; ++i) {
        if (buffer.data[i] != '\n') { continue; }
        uint_DynArr_insert(&textedit->linebreaks, (uint)i);
    }
    uint_DynArr_insert(&textedit->linebreaks, (uint)textedit->buffer->size);
    textedit->line_amount = (int)textedit->linebreaks.size -2;

    // Note: For more performance implement "update_linebreaks_around_cursor".
}


void GUI_TextEdit_enable(TextEdit *textedit, strview_t initial_buffer) {
    textedit->enabled = true;
    textedit->cursor = 0;
    strbuf_assign(&textedit->buffer, initial_buffer);
    GUI_TextEdit__update_linebreaks(textedit, 0);
}

void GUI_TextEdit_disable(TextEdit *textedit) {
    textedit->enabled = false;
}

// void GUI_TextEdit_get_buffer(TextEdit *textedit) { }


bool GUI_TextEdit_is_unicode_in_range(int unicode) {
    return (unicode >= 32 && unicode <= 126)
        || (unicode >= 160 && unicode <= 255);

    // https://en.wikipedia.org/wiki/List_of_Unicode_characters
}

void GUI_TextEdit__cursor_down(TextEdit *textedit, int cursor_line) {
    int break1 = -1;
    if (cursor_line -1 >= 0) { 
        break1 = (int)textedit->linebreaks.items[cursor_line];
    }
    int break2 = (int)textedit->linebreaks.items[cursor_line+1];
    int break3;
    if (cursor_line +1 >= (int)textedit->linebreaks.size) {
        break3 = textedit->buffer->size;
    } else {
        break3 = (int)textedit->linebreaks.items[cursor_line+2];
    }

    int new_cursor = int_min(
        textedit->cursor + break2 - break1,
        break3
    );
    textedit->cursor = new_cursor;
}

void GUI_TextEdit__cursor_up(TextEdit *textedit, int cursor_line) {
    int break1 = -1;
    if (cursor_line -1 > 0) { 
        break1 = (int)textedit->linebreaks.items[cursor_line-1];
    }
    int break2 = (int)textedit->linebreaks.items[cursor_line];

    int new_cursor = int_min(
            textedit->cursor - (break2 - break1),
            break2
        );
    textedit->cursor = new_cursor;
}

void GUI_TextEdit__update_selection(
    TextEdit *textedit, bool shift, int cursor_line
) {
    // shift    + no_selecting -> start selecting
    // shift    + selecting    -> continue selecting
    // no shift + selecting    -> stop selecting
    if (shift) {
        if (!textedit->is_selecting) {
            textedit->selection_origin = textedit->cursor;
            textedit->selection_line = cursor_line;
            textedit->is_selecting = true;
        }
    }
    else if (textedit->is_selecting) {
        textedit->is_selecting = false;
    }
}

void GUI_TextEdit__discard_undo_log(TextEdit *textedit) {
    for (uint i = 0; i < textedit->editlog.cursor; ++i) {
        TextEditRing_pop_head(&textedit->editlog.ring);
    }
    textedit->editlog.cursor = 0;
}

void GUI_TextEdit__move_by_world(TextEdit *textedit, int dir) {
    strview_t delimiters = cstr("\n(){};.,\"\'#");
    int new_cursor;
    bool found_space = false;

    if (dir > 0) {
        new_cursor = textedit->buffer->size;
        for (int i = textedit->cursor+1; i < textedit->buffer->size; ++i) {
            if (textedit->buffer->cstr[i] == ' ') {
                found_space = true;
                continue;
            }
            for (int k = 0; k < delimiters.size; ++k) {
                if (textedit->buffer->cstr[i] == delimiters.data[k] || found_space) {
                    new_cursor = i;
                    goto exit_loop;
                }
            }
        }
    } else {
        // Only check for Char-Space diff when going LEFT like GTK.
        bool found_char = false; 
        new_cursor = 0;
        for (int i = textedit->cursor-2; i >= 0; --i) {
            if (textedit->buffer->cstr[i] == ' ') {
                if (found_char) {
                    new_cursor = i +1;
                    goto exit_loop;
                }
                found_space = true;
                continue;
            }
            for (int k = 0; k < delimiters.size; ++k) {
                if (textedit->buffer->cstr[i] == delimiters.data[k] || found_space) {
                    new_cursor = i +1;
                    goto exit_loop;
                }
            }
            found_char = true;
        }
    }
    exit_loop:
    textedit->cursor = new_cursor;
}

void GUI_TextEdit__apply_change(
    TextEdit *textedit, const TextEdit_Change *change, bool undo
) {
    // (undo ? !change->is_insert : change->is_insert) -> XOR.
    if (undo ^ change->is_insert) {
        strbuf_insert_at_index(
            &textedit->buffer, change->start, strview_of_buf(change->buffer));
        textedit->cursor += change->buffer->size;
    } else {
        strbuf_pop_at_index(
                &textedit->buffer, change->start, change->buffer->size);
        textedit->cursor = change->start;
    }
    GUI_TextEdit__update_linebreaks(textedit, 0);
}

void GUI_TextEdit__insert(TextEdit *textedit, int start, strview_t str) {
    // Discard undo history to create a 'new branch'.
    GUI_TextEdit__discard_undo_log(textedit);

    TextEditRing_extend_head_force(&textedit->editlog.ring);
    TextEdit_Change *change = TextEditRing_get_head(&textedit->editlog.ring);
    if (change == NULL) { return; }

    change->is_insert = true;
    change->start = start;
    strbuf_assign(&change->buffer, str);
    strbuf_shrink(&change->buffer);
    GUI_TextEdit__apply_change(textedit, change, false);
}

void GUI_TextEdit__delete_chunk(TextEdit *textedit, int start, int size) {
    TextEditRing_extend_head_force(&textedit->editlog.ring);
    TextEdit_Change *change = TextEditRing_get_head(&textedit->editlog.ring);
    if (change == NULL) { return; }

    change->is_insert = false;
    change->start = start;
    strview_t chunk =
            strview_sub(strview_of_buf(textedit->buffer), start, start+size);
    strbuf_assign(&change->buffer, chunk);
    strbuf_shrink(&change->buffer);
    GUI_TextEdit__apply_change(textedit, change, false);
}

void GUI_TextEdit__delete_selection(TextEdit *textedit) {
    int start, end;
    if (textedit->cursor > textedit->selection_origin) {
        start = textedit->selection_origin;
        end = textedit->cursor;
    } else {
        start = textedit->cursor;
        end = textedit->selection_origin;
    }
    //strbuf_pop_at_index(&textedit->buffer, start, end-start);
    textedit->is_selecting = false;
    //textedit->cursor = start;
    
    GUI_TextEdit__delete_chunk(textedit, start, end -start);
}

void GUI_TextEdit_draw(TextEdit *textedit, Rect2 view_rect) {
    BeginTextureMode(GUI.aux_texture);
    DrawRectangleRec(view_rect.rect, GRAY);

    float line_spacing = 2;
    float line_height = GUI.font_size + line_spacing;
    float number_padding = fmaxf(3, (GUI.font_width + GUI.font_spacing)
        * (ceilf(((float)textedit->line_amount +1) / 10.f) +1));

    // scrolling

    if (GetMouseWheelMove() != 0) {
        textedit->scroll -= GetMouseWheelMove() > 0 ? 1 : -1;
        textedit->scroll =
            int_max(0, int_min((int)textedit->line_amount, textedit->scroll));

        // keep cursor on view
        if (textedit->cursor_line < textedit->scroll) {
            GUI_TextEdit__cursor_down(textedit, textedit->cursor_line);
            ++textedit->cursor_line;
        }
        if (textedit->cursor_line > textedit->scroll
                + (int)(view_rect.height / line_height)
        ) {
            GUI_TextEdit__cursor_up(textedit, textedit->cursor_line);
            --textedit->cursor_line;
        }
    }
    int scroll = textedit->scroll;

    /*
     * Some of these variables only need to be updated on cursor change, however
     * for now it seems the computation required is negligible.
     */
    // get cursor line

    int cursor_line = -1;
    int chars_until_cursor_line = 0;
    int chars_until_selection = 0;
    for (size_t i = 0; i < textedit->linebreaks.size; ++i) {
        bool must_break = true;
        if (textedit->cursor > (int)textedit->linebreaks.items[i]) {
            cursor_line = (int)i-1;
            chars_until_cursor_line = (int)textedit->linebreaks.items[i];
            if (i != 0) {
                ++chars_until_cursor_line; // ???
            }
            must_break = false;
        }
        if (textedit->is_selecting &&
            textedit->selection_origin > (int)textedit->linebreaks.items[i]
        ) {
            chars_until_selection = (int)textedit->linebreaks.items[i];
            if (i != 0) {
                ++chars_until_selection; // ???
            }
            must_break = false;
        }
        if (must_break) {
            break;
        }
    }
    ++cursor_line;

    // draw selection area

    if (textedit->is_selecting) {
        /*
         * Three cases:
         * 1. Selection starts and ends on the same line (1 rect).
         * 2. Selection starts and ends on adjacent lines (2 rects).
         * 3. Selection start and end are on different non adjacent lines
         *    (n rects).
         */
        int select_start, select_end, line_start, line_end,
            chars_til_start, chars_til_end;
        if (textedit->cursor > textedit->selection_origin) {
            line_start      = textedit->selection_line;
            line_end        = cursor_line;
            chars_til_start = chars_until_selection;
            chars_til_end   = chars_until_cursor_line;
            select_start    = textedit->selection_origin - chars_til_start;
            select_end      = textedit->cursor - chars_til_end;
        } else {
            line_start      = cursor_line;
            line_end        = textedit->selection_line;
            chars_til_start = chars_until_cursor_line;
            chars_til_end   = chars_until_selection;
            select_start    = textedit->cursor - chars_til_start;
            select_end      = textedit->selection_origin - chars_til_end;
        }
        line_start -= scroll;
        line_end   -= scroll;
        float start_x = view_rect.x + number_padding +
                (float)(select_start) * (GUI.font_width + GUI.font_spacing);
        float end_x = view_rect.x + number_padding +
                (float)(select_end) * (GUI.font_width + GUI.font_spacing);
        Rectangle rect;

        if (line_start == line_end) {
            rect = (Rectangle) {
                start_x,
                view_rect.y + (float)line_start * (GUI.font_size + line_spacing),
                end_x - start_x, GUI.font_size
            };
            DrawRectangleRec(rect, BLUE);
        }
        else {
            rect = (Rectangle) {
                start_x,
                view_rect.y + (float)line_start * (GUI.font_size + line_spacing),
                view_rect.x + view_rect.width - start_x, GUI.font_size
            };
            DrawRectangleRec(rect, BLUE);
            rect = (Rectangle) {
                view_rect.x + number_padding,
                view_rect.y + (float)line_end * (GUI.font_size + line_spacing),
                end_x - (view_rect.x + number_padding), GUI.font_size
            };
            DrawRectangleRec(rect, BLUE);
            for (int i = line_start+1; i < line_end; ++i) {
                rect = (Rectangle) {
                    view_rect.x + number_padding,
                    view_rect.y + (float)i * (GUI.font_size + line_spacing),
                    view_rect.x + view_rect.width, GUI.font_size
                };
                DrawRectangleRec(rect, BLUE);
            }
        }
        DrawRectangleRec((Rectangle) {
            view_rect.x + number_padding +
                (float)(textedit->selection_origin - chars_until_selection)
                    * (GUI.font_width + GUI.font_spacing),
            view_rect.y +
                (float)(textedit->selection_line -scroll) * (GUI.font_size + line_spacing),
            2, GUI.font_size }, WHITE);
    }

    // draw buffer

    strview_t source = strview_of_buf(textedit->buffer);
    strview_t line = strview_of_buf(textedit->buffer);
    for (int i = scroll, k = 0; i < (int)textedit->linebreaks.size-1; ++i, ++k)
    {
        line.data = source.data + textedit->linebreaks.items[i]
                    + (i != 0 ? 1 : 0);
        line.size = (int)textedit->linebreaks.items[i+1]
                    - (int)textedit->linebreaks.items[i];
        if (i == (int)textedit->linebreaks.size -2) { --line.size; }

        DrawTextEx_strview(
            GUI.font,
            line,
            (Vector2) {view_rect.x + number_padding,
            view_rect.y + line_height * (float)k},
            GUI.font_size,
            GUI.font_spacing,
            0,
            BLACK
        );
    }

    // draw numbers

    static strbuf_space_t(16) _aux_str = STRBUF_STATIC_INIT(16);
    strbuf_t *aux_str = (strbuf_t*)(&_aux_str);

    for (uint i = (uint)scroll, k = 0;
            k <= (uint)ceilf(view_rect.height/line_height); ++i, ++k)
    {
        if (i <= (uint)textedit->line_amount) {
            strbuf_printf(&aux_str, "%d", i);
        } else {
            strbuf_assign(&aux_str, cstr("~"));
        }

        DrawTextEx_strview(
            GUI.font,
            strview_of_buf(aux_str),
            (Vector2) {view_rect.x, view_rect.y + line_height * (float)k},
            GUI.font_size,
            GUI.font_spacing,
            0,
            BLACK
        );
    }

    // draw cursor

    int screen_cursor = textedit->cursor - chars_until_cursor_line;

    DrawRectangleRec((Rectangle) {
        view_rect.x + number_padding +
        (float)(screen_cursor) * (GUI.font_width + GUI.font_spacing),
        view_rect.y +
        (float)(cursor_line -scroll) * (GUI.font_size + line_spacing),
        2, GUI.font_size }, WHITE);

    DrawRectangleLinesEx(view_rect.rect, 1, BLACK);
    EndTextureMode();

    // controls

    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    if (IsKeyPressed(KEY_LEFT) && (textedit->cursor > 0)) {
        GUI_TextEdit__update_selection(textedit, shift, cursor_line);
        if (control) GUI_TextEdit__move_by_world(textedit, -1);
        else --textedit->cursor;
    }

    if (IsKeyPressed(KEY_RIGHT) && (textedit->cursor < textedit->buffer->size)
    ) {
        GUI_TextEdit__update_selection(textedit, shift, cursor_line);
        if (control) GUI_TextEdit__move_by_world(textedit, +1);
        else ++textedit->cursor;
    }

    if (IsKeyPressed(KEY_DOWN) && (cursor_line < (int)(textedit->line_amount))
    ) {
        GUI_TextEdit__update_selection(textedit, shift, cursor_line);
        GUI_TextEdit__cursor_down(textedit, cursor_line);
    }

    if (IsKeyPressed(KEY_UP) && (cursor_line > 0)) {
        GUI_TextEdit__update_selection(textedit, shift, cursor_line);
        GUI_TextEdit__cursor_up(textedit, cursor_line);
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (textedit->is_selecting) {
            GUI_TextEdit__delete_selection(textedit);
        } else if (textedit->cursor > 0) {
            GUI_TextEdit__delete_chunk(textedit, textedit->cursor-1, 1);
        }
    }

    char new_char[2] = { '\0', '\0' };
    int codepoint;
    while ((codepoint = GetCharPressed())) // write text
    { 
        if (!GUI_TextEdit_is_unicode_in_range(codepoint)) continue;

        if (textedit->is_selecting) {
            GUI_TextEdit__delete_selection(textedit);
        }

        new_char[0] = (char)codepoint;
        GUI_TextEdit__insert(textedit, textedit->cursor, cstr(new_char));
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (textedit->is_selecting) {
            GUI_TextEdit__delete_selection(textedit);
        }

        new_char[0] = (char)'\n';
        GUI_TextEdit__insert(textedit, textedit->cursor, cstr(new_char));
        return;
    }

    // paste
    if (control && IsKeyPressed(KEY_V)) {
        const char *clipboard_cstr = GetClipboardText();
        strview_t clipboard = cstr(clipboard_cstr);
        if (clipboard.size > 0) {
            if (textedit->is_selecting) {
                GUI_TextEdit__delete_selection(textedit);
            }

            GUI_TextEdit__insert(textedit, textedit->cursor, clipboard);
        }
    }

    // undo & redo
    if (control && shift && IsKeyPressed(KEY_Z)) {
        TextEdit_Change *change = TextEditRing_get_from_head(
                &textedit->editlog.ring,
                textedit->editlog.cursor -1);
        if (change != NULL) {
            GUI_TextEdit__apply_change(textedit, change, false);
            --textedit->editlog.cursor;
            textedit->is_selecting = false;
        }
    }
    else if (control && IsKeyPressed(KEY_Z)) {
        TextEdit_Change *change = TextEditRing_get_from_head(
                &textedit->editlog.ring,
                textedit->editlog.cursor);
        if (change != NULL) {
            GUI_TextEdit__apply_change(textedit, change, true);
            ++textedit->editlog.cursor;
            textedit->is_selecting = false;
        }
    }

    // scroll to cursor
    while (cursor_line < textedit->scroll) {
        --textedit->scroll;
    }
    while (cursor_line > textedit->scroll + (int)(view_rect.height / line_height)) {
        ++textedit->scroll;
    }
    textedit->cursor_line = cursor_line;
}

void GUI_draw_all(WuiState *state) {
    BeginTextureMode(GUI.final_texture);
        ClearBackground(BLANK);
    EndTextureMode();

    Rect2 view_rect;

    // Breakpoint

    view_rect = (Rect2) {{ 20, 250, 120, 120 }};
    GuiBreakpointView_draw(state, view_rect);
    BeginTextureMode(GUI.final_texture);
        DrawTextureRec_flipped(GUI.aux_texture.texture, view_rect.rect, view_rect.pos, WHITE);
    EndTextureMode();

    // TextEdit

    view_rect = (Rect2) {{ 150, 150, 240, 150 }};
    GUI_TextEdit_draw(&GUI.textedit, view_rect);
    BeginTextureMode(GUI.final_texture);
        DrawTextureRec_flipped(GUI.aux_texture.texture, view_rect.rect, view_rect.pos, WHITE);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawTexture_flipped(GUI.final_texture.texture, 0, 0, WHITE);
        DrawFPS(0,0);

        Rectangle btn = { 20, 20, 300, 20 };
        if (GuiButton(btn, "Press Me!")) {
            printf("Thank you\n");
        }
        btn.y += btn.height;
        if (GuiButton(btn, "Press Me Too!")) {
            printf("Thank you twise\n");
        }

        GUI_draw_popups();

    EndDrawing();
}

#endif // !GUI
