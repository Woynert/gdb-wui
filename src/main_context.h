#ifndef MAIN_CONTEXT_H
#define MAIN_CONTEXT_H

#include "cli_prompt.h"
#include "raylib_extra.h"
#include "wui_state.h"
#include "ipc.h"

typedef enum WIDGET {
    WIDGET_NONE,
    WIDGET_BREAKPOINTS,
    WIDGET_LOCALS,
    WIDGET_TEXTEDIT,
    WIDGET_MAX,
} WIDGET;


// MOVEME
typedef struct Widget {
    bool is_focused; /* Draw only, see ctx.widget_focused_id. */
    WIDGET type;
    Rect2 area;

    bool is_scrollable;
    int scroll_px;
    int max_scroll_px_h;
} Widget;


#define WIDGET_INVALID ((Widget) { .id = -10, .area = (Rect2) {{ 0 }} })

#define DYNA__TYPE Widget
#include "containers/da.h"
// MOVEME


typedef struct Ctx {
    WuiState wui_state;
    IPCCtx ipc_ctx;
    IPCReader reader;
    CliPrompt cli_prompt;

    struct {
        Widget_Dyna widget_stack;
        int widget_focused_id;
    };
} Ctx;


void ctx_init(Ctx *ctx) {
    *ctx = (Ctx) { 0 };
    WuiState_init(&ctx->wui_state);
    IPCReader_init(&ctx->reader);
    CliPrompt_init(&ctx->cli_prompt);

    // MOVEME
    ctx->widget_stack = Widget_Dyna_create();
    Widget_Dyna_append(&ctx->widget_stack, (Widget) {
        .type = WIDGET_LOCALS,
        .area = (Rect2) {{ 20, 20, 500, 500 }},
    });
    Widget_Dyna_append(&ctx->widget_stack, (Widget) {
        .type = WIDGET_BREAKPOINTS,
        .area = (Rect2) {{ 20, 450, 200, 120 }},
    });
    Widget_Dyna_append(&ctx->widget_stack, (Widget) {
        .type = WIDGET_TEXTEDIT,
        .area = (Rect2) {{ 650, 200, 187, 150 }},
    });
    // MOVEME
}

void ctx_free(Ctx *ctx) {
    WuiState_free(&ctx->wui_state);
    IPCReader_free(&ctx->reader);
    //CliPrompt_free(&ctx->cli_prompt);

    Widget_Dyna_free(&ctx->widget_stack);

    *ctx = (Ctx) { 0 };
}

/*
struct Widget {
};

   {
    (Widget) { id, area },
    (Widget) { id, area },
    (Widget) { id, area },
   }
   */


#endif // !MAIN_CONTEXT_H
