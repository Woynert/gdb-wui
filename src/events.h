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

/* @Note: Might wanna not use recursion. */
void get_symbol_absolute_name(WuiState *w, uint node_id, strbuf_t **buf);
void get_symbol_absolute_name(WuiState *w, uint node_id, strbuf_t **buf) {

    uint parent_node_id = 0;
    int res = SymbolTree_get_parent(&w->locals, node_id, &parent_node_id);
    bool has_parent = res == 0;
    printfd("Found parent %d", has_parent);

    if (has_parent) {
        printfd("Node %d has parent %d", node_id, parent_node_id);

        get_symbol_absolute_name(w, parent_node_id, buf);

        strbuf_append_printf(buf, ".");
    }

    WuiSymbol *symbol = SymbolTree_get(&w->locals, node_id);
    if (symbol == NULL) {
        printfd("ERR: No symbol (node id %d)", node_id);
        return;
    }

    strbuf_append(buf, strbuf_view2(&symbol->symbol_name));
    return;
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
    strbuf_t *aux_str_name = strbuf_create_empty(0, NULL);

    get_symbol_absolute_name(&ctx->wui_state, node_id, &aux_str_name);
    printfd("Absolute name '%"PRIstr"'", PRIstrarg(strbuf_view2(aux_str_name)));


    strbuf_printf(&aux_str, "py woy_query_symbol(\"%"PRIstr"\", False)\n",
            PRIstrarg(strbuf_view2(aux_str_name)));

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
    strbuf_destroy(&aux_str_name);
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
