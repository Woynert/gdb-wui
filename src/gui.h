#ifndef GUI_H
#define GUI_H

#include "stdio.h"
#include "math.h"
#include "limits.h"

#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "raylib_extra.h"
#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"
#include "portable_utils.h"

//#include "ipc.h"
#include "wui_state.h"
#include "textedit.h"
#include "main_context.h"
#include "events.h"


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


struct {
    Font font;
    float font_size;
    float font_width;
    float font_spacing; /* Space between chars. */

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

    Textedit textedit;
} GUI;

void GUI_init_global_context(void) {
    STRBUF_STATIC_INIT2(GUI_OPTIONS_STRING_LEN, GUI.context_menu._options);

    // TODO: what about multiscreen setups?
    GUI.aux_texture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    GUI.final_texture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    textedit_init(&GUI.textedit);

    //LoadFont
    GUI.font_spacing = 1;
    GUI.font_size = 15;

    // Check ranges at (https://unicode.org/charts/nameslist/mainList.html).
    int ranges[] = {
        32,      127,     // Basic latin
        0x00A1,  0x00FF,  // C1 Controls and Latin-1 Supplement
        0x0100,  0x017F,  // Latin Extended-A
        0x0180,  0x024F,  // Latin Extended-B
        0x1F300, 0x1F5FF, // Miscellaneous Symbols and Pictographs
        0x1F600, 0x1F64F, // Emoticons
    };

    {
        int_Dyna codepoints = int_Dyna_create();
        int_Dyna_append(&codepoints, 0xFFFD); // (�) codepoint

        for (int i = 0; i < (int)(sizeof(ranges)/sizeof(ranges[0])); i += 2) {
            for (int j = ranges[i]; j <= ranges[i+1]; ++j) {
                int_Dyna_append(&codepoints, j);
            }
        }

        GUI.font = LoadFontEx(
            "assets/IosevkaFixed-Regular.ttf",
            (int)GUI.font_size, codepoints.items, (int)codepoints.size);
        int_Dyna_free(&codepoints);
    }
    //SetTextureFilter(draw_ctx.font.texture, TEXTURE_FILTER_POINT);

    int w1 = (int)MeasureTextEx(GUI.font, "W",
            (float)GUI.font_size, GUI.font_spacing).x;
    int w2 = (int)MeasureTextEx(GUI.font, "@",
            (float)GUI.font_size, GUI.font_spacing).x;
    int w3 = (int)MeasureTextEx(GUI.font, "_",
            (float)GUI.font_size, GUI.font_spacing).x;
    GUI.font_width = (float)int_max(w1, int_max(w2, w3));
}

void GUI_cleanup(void) {
    textedit_free(&GUI.textedit);
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


void GUI_draw_breakpoints(Ctx *ctx, WuiState *state, Widget widget) {

    bool is_focused = widget.id == ctx->widget_focused_id;
    Rect2 view_rect = widget.area;

    BeginTextureMode(GUI.aux_texture);
    DrawRectangleRec(view_rect.rect, GRAY);

    float line_height = GUI.font_size + 2;
    for (int i = 0; i < state->breakpoints.size; ++i) {
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

    if (is_focused
        && CheckCollisionPointRec(GetMousePosition(), view_rect.rect)
        && BetterMouse_is_pressed(MOUSE_BUTTON_RIGHT)
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

bool GUI__symbol_is_expandable(int symbol_type) {
    return symbol_type == SYMBOL_TYPE_STRUCT;
    /* Symbol is ARRAY o pointer to STRUCT. */
}

void GUI_draw_symbol_tree(Ctx *ctx, SymbolTree *tree, Widget widget) {
    bool is_focused = widget.id == ctx->widget_focused_id;
    Rect2 view_rect = widget.area;

    BeginTextureMode(GUI.aux_texture);
    DrawRectangleRec(view_rect.rect, GRAY);

    const float LINE_HEIGHT = GUI.font_size;
    const float PAD = 5;
    const int col_width = (int)(GUI.font_width + GUI.font_spacing);
    const int max_cols = (int)(view_rect.width - PAD * 2) / col_width;
    Vector2 text_pos = { view_rect.x + PAD, view_rect.y };
    Rect2 selectable_area = {{ view_rect.x, text_pos.y, view_rect.width, LINE_HEIGHT }};

    SymbolTree_Iterator it = { 0 };
    while(SymbolTree_get_next(tree, &it)) {
        WuiSymbol *symbol = it.item;
        strbuf_empty(&gui_str);

        /* Depth. */
        for (int i = 0; i < it.depth; ++i) { strbuf_append(&gui_str, cstr_SL("   ")); }

        strbuf_append_printf(&gui_str, "%s%s = %s",
            GUI__symbol_is_expandable(symbol->basic_type) ? "> " : "  ",
            symbol->symbol_name.cstr,
            symbol->value.cstr
        );
        int prev_size = gui_str->size;
        strbuf_append_printf(&gui_str, "%s%s",
            symbol->type_name.cstr,
            symbol->basic_type == SYMBOL_TYPE_PTR ? " [pointer]" : ""
        );
        strview_t line1 = {
            .data = gui_str->cstr,
            .size = prev_size,
        };
        strview_t line2 = {
            .data = line1.data + line1.size,
            .size = gui_str->size - line1.size,
        };

        int cols = utf8_codepoint_count_strview(line1);
        int type_cols = utf8_codepoint_count_strview(line2);
        bool it_all_fits = (max_cols - cols) >= type_cols;
        float prev_x = text_pos.x;

        /* Mouse hovering. */

        selectable_area.height = LINE_HEIGHT * (it_all_fits ? 1 : 2);
        Color selectable_area_color;

        if (is_focused && CheckCollisionPointRec(GetMousePosition(), selectable_area.rect))
        {
            selectable_area_color = BLUE;
            if (BetterMouse_is_pressed(MOUSE_LEFT_BUTTON)) {
                WuiState_queue_event_symbol_query(&ctx->wui_state,
                    (EventSymbolQuery) {
                        .symbol_node_id = it.node_id,
                    }
                );
                // DELME
                //int err = IPC_write_cmd(&ipc_ctx, cstr("py woy_locals()\n"));
                //if (err == 0) {
                    //IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_READ_WOY_LOCALS, &wui_state, 0);
                    //err = IPC_write_cmd(&ipc_ctx, cstr("echo\n"));
                    //if (err == 0) {
                        //IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
                    //}
                //}
                // DELME
            }
        } else {
            selectable_area_color = (it.node_id % 2) == 0 ? GOLD : ORANGE;
        }
        DrawRectangleRec(selectable_area.rect, selectable_area_color);

        /* General info. */

        DrawTextEx_strview(GUI.font, line1, text_pos, GUI.font_size, GUI.font_spacing, 0, BLACK);

        /* Data type. */

        text_pos.x = view_rect.x + view_rect.width - PAD - (float)(type_cols * col_width);
        if (!it_all_fits) {
            text_pos.y += LINE_HEIGHT;
        }

        DrawTextEx_strview(GUI.font, line2, text_pos, GUI.font_size, GUI.font_spacing, 0, DARKGRAY);

        text_pos.x = prev_x;
        text_pos.y += LINE_HEIGHT;
        selectable_area.y = text_pos.y;
    }

    DrawRectangleLinesEx(view_rect.rect, 1, BLACK);
    EndTextureMode();
}


void GUI_draw_locals(Ctx *ctx, WuiState *state, Widget widget) {
    GUI_draw_symbol_tree(ctx, &state->locals, widget);
}

void GUI_draw_popups(Ctx *ctx, Widget widget) {
    if (GUI.curr_popup == POPUP_NONE) { return; }
    else if (GUI.curr_popup == POPUP_CONTEXT_MENU) {
        if (widget.id != ctx->widget_focused_id) { return; }
        Vector2 mouse = GetMousePosition();
        strview_t all_lines = strbuf_view2(&GUI.context_menu.options);
        strview_t line = { 0 };
        int line_height = (int)GUI.font_size + GUI_POPUP_PAD;
        Rect2 rect = {{
            widget.area.x, // GUI.context_menu.origin.x,
            widget.area.y, // GUI.context_menu.origin.y,
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

        if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {
            printf("Pressed option %d\n", hover_option_id);
            GUI_close_popup(hover_option_id);
        }
    }
}


/// @returns widget id.
Widget GUI_register_widget(Ctx *ctx, Rect2 rect) {
    Widget w = { .area = rect, .id = ctx->widget_stack.size };
    Widget_Dyna_append(&ctx->widget_stack, w);
    return w;
}


void GUI__calculate_focus(Ctx *ctx) {
    /* Check stack from top to bottom. */

    for (int i = ctx->widget_stack.size-1; i > -1; --i) {
        Widget widget = ctx->widget_stack.items[i];

        if (CheckCollisionPointRec(GetMousePosition(), widget.area.rect)) {
            ctx->widget_focused_id = widget.id;
            return;
        }
    }
    ctx->widget_focused_id = -1;
}


void GUI_draw_all(Ctx *ctx, WuiState *state) {
    BeginTextureMode(GUI.final_texture);
        ClearBackground(BLANK);
    EndTextureMode();

    // Widget stack focus calculation.
    Widget_Dyna_clear_preserve(&ctx->widget_stack);
    Widget widget_locals     = GUI_register_widget(ctx, (Rect2) {{ 20, 20, 500, 500 }});
    Widget widget_breakpoint = GUI_register_widget(ctx, (Rect2) {{ 20, 450, 200, 120 }});
    Widget widget_textedit   = GUI_register_widget(ctx, (Rect2) {{ 650, 200, 187, 150 }});

    // @Note: This "popup" system eventually has to be refactored away.
    // Include popup if opened.
    Widget widget_popup = WIDGET_INVALID;
    bool popup_is_open = GUI.curr_popup != POPUP_NONE;
    if (popup_is_open) {
        widget_popup = GUI_register_widget(
            ctx,
            (Rect2) {{
                GUI.context_menu.origin.x, GUI.context_menu.origin.y,
                (float)GetScreenWidth(), (float)GetScreenHeight()
            }}
        );
    }


    GUI__calculate_focus(ctx);

    if (popup_is_open && ctx->widget_focused_id != widget_popup.id) {
        GUI_close_popup(-1);
    }

    // Locals

    GUI_draw_locals(ctx, state, widget_locals);
    BeginTextureMode(GUI.final_texture);
    DrawTextureRec_flipped(GUI.aux_texture.texture,
            widget_locals.area.rect, widget_locals.area.pos, WHITE);
    EndTextureMode();

    // Breakpoint

    GUI_draw_breakpoints(ctx, state, widget_breakpoint);
    BeginTextureMode(GUI.final_texture);
    DrawTextureRec_flipped(GUI.aux_texture.texture,
            widget_breakpoint.area.rect, widget_breakpoint.area.pos, WHITE);
    EndTextureMode();

    // TextEdit

    //textedit_draw(&GUI.textedit, widget_textedit.area, GUI.font, GUI.font_size,
                //GUI.font_spacing, GUI.font_width);
    //BeginTextureMode(GUI.final_texture);
    //DrawTextureRec_flipped(GUI.aux_texture.texture,
            //widget_textedit.area.rect, widget_textedit.area.pos, WHITE);
    //EndTextureMode();

    /*
    view_rect = (Rect2) {{ 150, 150, 187, 150 }};
    textedit_draw(&GUI.textedit, view_rect);
    BeginTextureMode(GUI.final_texture);
        DrawTextureRec_flipped(GUI.aux_texture.texture, view_rect.rect, view_rect.pos, WHITE);
    EndTextureMode();
    */

    BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawTexture_flipped(GUI.final_texture.texture, 0, 0, WHITE);
        DrawFPS(0,0);


        //view_rect = (Rect2) {{ 250, 300, 187, 150 }};
        //textedit_draw(&GUI.textedit, view_rect, GUI.font, GUI.font_size,
                //GUI.font_spacing, GUI.font_width);

        textedit_draw(&GUI.textedit, widget_textedit.area, GUI.font, GUI.font_size,
                GUI.font_spacing, GUI.font_width);

        if (popup_is_open) {
            GUI_draw_popups(ctx, widget_popup);
        }
    /* EndDrawing(); Do not call EndDrawing here. See main loop. */
}

#endif // !GUI_H
