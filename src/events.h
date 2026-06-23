#ifndef EVENTS_H
#define EVENTS_H

#include "wui_state.h"
#include "portable_utils.h"
#include "stdbool.h"
#include "ipc.h"
#include "stdio.h"
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

    if (symbol->is_expanded) {
        SymbolTree_destroy_children(&ctx->wui_state.locals, node_id);
        symbol->is_expanded = false;
        return;
    }

    /* @note: Maybe move this down if we have early returns (failures). */
    symbol->is_expanded = true; 

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


// TODO: Think of a better name.
int update_file_view_from_file_line_query(Ctx *ctx) {
    WuiState *w = &ctx->wui_state;

    // 1. Check we have valid location.
    if (!w->has_valid_location) {
        printfd("ERR: No valid location.");
        return -1;
    }

    // 2. Check file exists in disk.
    FILE *file = fopen(w->curr_file_path->cstr, "rb");
    if (file == NULL) {
        printfd("ERR:");
        perror("fopen");
        return -1;
    }

    // 3. Read file size.
    if (fseek(file, 0, SEEK_END) != 0) {
        printfd("ERR:");
        perror("fseek");
        goto out;
    }
    int size = (int)ftell(file);
    fseek(file, 0, SEEK_SET);

    // 3. Read whole file into buffer.
    strbuf_t **buf = &w->file_contents_tmp_buf;
    strbuf_empty(buf);
    strbuf_grow(buf, size);
    int bytes_read = (int)fread(&(*buf)->cstr, sizeof(char), (size_t)size, file);
    if (bytes_read <= 0) {
        printfd("ERR: Couldn't read.");
        if (bytes_read < 0) { perror("fread"); }
        goto out;
    }
    (*buf)->size = bytes_read;

    if ((0)) {
        printfd("Got it! It reads (bytes %d) [%"PRIstr"]",
            (*buf)->size, PRIstrarg(strbuf_view(buf)));
    }

    // 4. Update textedit buffer.
    textedit_set_buffer(&ctx->textedit, strbuf_view(buf));

    // 5. Reveal current line.
    textedit_reveal_line(&ctx->textedit, w->curr_line);
    textedit_highlight_line(&ctx->textedit, true, w->curr_line);

    return 0;

    out:
    {
        fclose(file);
        return -1;
    }
}


#endif // !EVENTS_H
