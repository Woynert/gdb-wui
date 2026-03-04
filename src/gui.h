#ifndef GUI

#include "stdio.h"

#include "raygui.h"
#include "raylib.h"
#include "raylib_extra.h"
#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"
#include "portable_utils.h"

#include "wui_state.h"

typedef struct {
    Font font;
    float font_size;
    float font_width;
    float font_spacing;

    RenderTexture2D aux_texture;
    RenderTexture2D final_texture;
    // TODO: overlay texture for Pop-Ups.
} GuiCtx;

void GuiCtx_init(GuiCtx *gui) {
    // TODO: what about multiscreen setups?
    gui->aux_texture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    gui->final_texture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    //LoadFont
    gui->font_spacing = 1;
    gui->font_size = 15;
    gui->font = LoadFontEx("./IosevkaFixed-Regular.ttf", (int)gui->font_size, 0, 250);
    //gui->font = LoadFontEx("./assets/RobotoMono-Regular.ttf", (int)gui->font_size, 0, 250);
    //SetTextureFilter(draw_ctx.font.texture, TEXTURE_FILTER_POINT);

    int w1 = (int)MeasureTextEx(gui->font, "W",
            (float)gui->font_size, gui->font_spacing).x;
    int w2 = (int)MeasureTextEx(gui->font, "@",
            (float)gui->font_size, gui->font_spacing).x;
    int w3 = (int)MeasureTextEx(gui->font, "_",
            (float)gui->font_size, gui->font_spacing).x;
    gui->font_width = (float)int_max(w1, int_max(w2, w3));

}

//typedef struct GuiBreakpointView {
    //int i;
//} GuiBreakpointView;


/* Notes:
 * Since strbuf empties the string if the space is insufficient (for static
 * strings) then we need to use a large enough buffer as a temporary container
 * and then copy just the size we need.
 */
strbuf_space_t(4096) _gui_str = STRBUF_STATIC_INIT(4096);
strbuf_t *gui_str = (strbuf_t*)(&_gui_str);

#define GUI_FONT_SIZE 10
#define GUI_TEXT_LINE_HEIGHT 13

void GuiBreakpointView_draw(GuiCtx *gui, WuiState *state, Rect2 view_rect) {
    //Rect2 view_rect = {{ 0, 0, 120, 120 }};

    BeginTextureMode(gui->aux_texture);
    DrawRectangleRec(view_rect.rect, GRAY);
    DrawRectangleLinesEx(view_rect.rect, 1, BLACK);

    float line_height = gui->font_size + 2;
    for (size_t i = 0; i < state->breakpoints.size; ++i) {
        WuiBreakpoint *breakpoint = &state->breakpoints.items[i];

        strbuf_printf(&gui_str, "%d%s %s %s",
            breakpoint->id,
            breakpoint->enabled ? "" : " OFF",
            breakpoint->location.cstr,
            breakpoint->file.cstr);

        DrawTextEx(gui->font, gui_str->cstr,
            (Vector2){ view_rect.x, view_rect.y + (float)i * line_height},
            gui->font_size, gui->font_spacing, BLACK);
    }
    EndTextureMode();
}

void GUI_draw_all(GuiCtx *gui, WuiState *state) {
    BeginTextureMode(gui->final_texture);
        ClearBackground(BLANK);
    EndTextureMode();

    Rect2 view_rect;
    view_rect = (Rect2) {{ 30, 100, 120, 120 }};
    GuiBreakpointView_draw(gui, state, view_rect);

    BeginTextureMode(gui->final_texture);
        DrawTextureRec_flipped(gui->aux_texture.texture, view_rect.rect, view_rect.pos, WHITE);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawTexture_flipped(gui->final_texture.texture, 0, 0, WHITE);
        DrawFPS(0,0);

        Rectangle btn = { 20, 20, 300, 20 };
        if (GuiButton(btn, "Press Me!")) {
            printf("Thank you\n");
        }
        btn.y += btn.height;
        if (GuiButton(btn, "Press Me Too!")) {
            printf("Thank you twise\n");
        }

    EndDrawing();
}

#endif // !GUI
