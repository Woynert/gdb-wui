#ifndef EVENTS_H
#define EVENTS_H

#include "wui_state.h"
#include "portable_utils.h"
#include "stdbool.h"
#include "ipc.h"
#include "main_context.h"
#include <assert.h>

void WuiState_queue_event_symbol_query(WuiState *w, EventSymbolQuery event) {
    Event_da_append(&w->events, (Event){ .type = EVENT_SYMBOL_QUERY, .event_symbol_query = event });
}

void WuiState_handle_event_symbol_query(Ctx *ctx, EventSymbolQuery event) {
    int err;
    uint node_id = event.symbol_node_id;

    // Get symbol name.
    WuiSymbol *symbol = SymbolTree_get(&ctx->wui_state.locals, node_id);
    if (symbol == NULL) {
        printfd("Err: Symbol (node id %d) not found.", node_id);
        return;
    }

    /* @todo: Have a temp string to use when needed instead. */
    strbuf_t *aux_str = strbuf_create_empty(0, NULL);

    strbuf_printf(&aux_str, "py woy_query_symbol(\"%"PRIstr"\", False)\n",
            PRIstrarg(strbuf_view2(&symbol->symbol_name)));

    printfd("Query is %"PRIstr"", PRIstrarg(strbuf_view(&aux_str)));

    err = IPC_write_cmd(&ctx->ipc_ctx, strbuf_view(&aux_str));
    ASSERT(err == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt,
            IPC_WAIT_DO_READ_WOY_LOCALS, &ctx->wui_state, node_id);

    // After calling a 'WOY API' command, clear the GDB previous command
    err = IPC_write_cmd(&ctx->ipc_ctx, cstr("echo\n"));
    ASSERT(err == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt,
            IPC_WAIT_DO_NOTHING, NULL, 0);

    strbuf_destroy(&aux_str);
}

void WuiState_process_events(Ctx *ctx) {
    WuiState *w = &ctx->wui_state;
    for (int i = 0; i < w->events.size; ++i) {
        Event event_broad = w->events.items[i];

        static_assert(EVENT_MAX == 1, "Enum changed.");
        switch (event_broad.type) {
            case EVENT_SYMBOL_QUERY:
            {
                EventSymbolQuery event = event_broad.event_symbol_query;
                printf("OMG I'm handling the EVENT_SYMBOL_QUERY event!\n");
                printf("Selected symbol node id: %d\n", event.symbol_node_id);
                WuiState_handle_event_symbol_query(ctx, event);
                printf("Handled.\n");
                break;
            }
            default: ASSERT(false);
        }
    }

    Event_da_clear_preserve(&w->events);
}


#endif // !EVENTS_H
