#ifndef EVENTS_H
#define EVENTS_H

#include "main_context.h"

void WuiState_queue_event_symbol_query(WuiState *w, EventSymbolQuery event);
void get_symbol_absolute_name(WuiState *w, uint node_id, strbuf_t **buf);
void WuiState_handle_event_symbol_query(Ctx *ctx, EventSymbolQuery event);
void WuiState_process_events(Ctx *ctx);
int update_file_view_from_file_line_query(Ctx *ctx);
void wait_request_get_file_and_line(Ctx *ctx);
void wait_request_get_locals(Ctx *ctx);
void SymbolTree_update(SymbolTree *base, SymbolTree *update);
void trigger_fileline_refresh(Ctx *ctx);
void process_update_fileline(Ctx *ctx);
int SymbolTree_find_node_by_full_path(SymbolTree *tree, strview_t target_symbol, SymbolTree_Iterator *out_it);
bool SymbolTree_node_has_children(SymbolTree *tree, SymbolTree_Iterator it);

#endif // !EVENTS_H
#include "have_lsp.h"
#if (defined EVENTS_H_IMPLEMENTATION || defined HAVE_LSP) && !defined EVENTS_H_DONE
#define EVENTS_H_DONE
/*
 * █ █▀▄▀█ █▀█ █   █▀▀ █▀▄▀█ █▀▀ █▄ █ ▀█▀ ▄▀█ ▀█▀ █ █▀█ █▄ █
 * █ █ ▀ █ █▀▀ █▄▄ ██▄ █ ▀ █ ██▄ █ ▀█  █  █▀█  █  █ █▄█ █ ▀█
 */


#include <assert.h>
#include "portable_utils.h"
#include "stdbool.h"
#include "stdio.h"
#include "ipc.h"


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


void wait_request_get_file_and_line(Ctx *ctx) {
    int error = IPC_write_cmd(&ctx->ipc_ctx, cstr("py woy_get_file_and_line()\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt, IPC_WAIT_DO_READ_WOY_FILE_LINE, &ctx->wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(&ctx->ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
}


void wait_request_get_locals(Ctx *ctx) {
    int error = IPC_write_cmd(&ctx->ipc_ctx,
    cstr("interpreter-exec mi \"-stack-list-variables --no-frame-filters 1\"\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt, IPC_WAIT_DO_READ_WOY_LOCALS, &ctx->wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(&ctx->ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
    /*
       Problem: How to update the existing locals?...
       Maybe we could detect whether we're on the same frame... If new frame detected then do not update?
       Or maybe we don't care about that but rather we build a new tree and then try to merge that to
       the new one. (We just need to keep what is expanded and what is not...)
   */
}


/// @returns Error.
int SymbolTree_find_node_by_full_path(SymbolTree *tree, strview_t target_symbol, SymbolTree_Iterator *out_it_readonly) {

   // @todo: Grab an auxiliar string from somewhere else.
   static strbuf_t *str_aux = NULL;
   if (str_aux == NULL) {
      str_aux = strbuf_create_empty(0, NULL);
   }
   // @todo!

   strbuf_t **str_symbol_path = &str_aux;
   strbuf_empty(str_symbol_path);

   int prev_depth = -1;
   SymbolTree_Iterator it = { 0 };

   while(SymbolTree_get_next(tree, &it)) {
      WuiSymbol *symbol = it.item;

      // Return or "navigate" up.
      if (prev_depth == -1) { prev_depth = it.depth; }
      for (int i = 0; i < prev_depth - it.depth; ++i) {
         strview_t removed_last_symbol = strbuf_view(str_symbol_path);
         (void)strview_split_last_delim(&removed_last_symbol, ".", false);
         strbuf_assign(str_symbol_path, removed_last_symbol);
      }

      // Add itself
      if (it.depth > 0) { strbuf_append(str_symbol_path, "."); }
      strbuf_append(str_symbol_path, strbuf_view2(&symbol->symbol_name));

      printfd("Comparing (%"PRIstr") == (%"PRIstr")",
            PRIstrarg(strbuf_view(str_symbol_path)), PRIstrarg(target_symbol));

      // Check for equality
      if (strview_compare(strbuf_view(str_symbol_path), target_symbol) == 0) {
         *out_it_readonly = it;
         return 0;
      }

      // Remove itself
      if (!SymbolTree_node_has_children(tree, it)) {
         strview_t removed_last_symbol = strbuf_view(str_symbol_path);
         (void)strview_split_last_delim(&removed_last_symbol, ".", false);
         strbuf_assign(str_symbol_path, removed_last_symbol);
      }

      prev_depth = it.depth;
   }

   return -1;
}


bool SymbolTree_node_has_children(SymbolTree *tree, SymbolTree_Iterator it) {
    SymbolTree_Iterator it2 = it;
    if (!SymbolTree_get_next(tree, &it2)) {
        return false;
    }
    return it2.depth > it.depth;
}


void wait_request_update_locals(Ctx *ctx, SymbolTree *tree) {

    strbuf_t **str_symbol_path = &ctx->aux_str1;
    strbuf_t **str_list        = &ctx->aux_str2;
    strbuf_t **str_query       = &ctx->aux_str3;

    strbuf_empty(str_symbol_path);
    strbuf_empty(str_list);
    strbuf_empty(str_query);

    // Build expanded symbols list.
    // @note. This could be cached but for now will look them up directly.

    int prev_depth = -1;

    SymbolTree_Iterator it = { 0 };

    while(SymbolTree_get_next(tree, &it)) {
        WuiSymbol *symbol = it.item;

        // Path "navigate" up.
        if (prev_depth == -1) { prev_depth = it.depth; }
        for (int i = 0; i < prev_depth - it.depth; ++i) {
            strview_t removed_last_symbol = strbuf_view(str_symbol_path);
            (void)strview_split_last_delim(&removed_last_symbol, ".", false);
            strbuf_assign(str_symbol_path, removed_last_symbol);
        }

        if (symbol->is_expanded && SymbolTree_node_has_children(tree, it)) {
            // Concatenate name.
            if (it.depth > 0) {

                // @note. Some symbols might contain '.' maybe use a different
                // separator and replace it later when making the query.

                strbuf_append(str_symbol_path, ".");
            }
            strbuf_append(str_symbol_path, strbuf_view2(&symbol->symbol_name));

            // Add to list.
            strbuf_append(str_list, "\"");
            strbuf_append(str_list, strbuf_view(str_symbol_path));
            strbuf_append(str_list, "\",");

            printfd("-> (%"PRIstr")", PRIstrarg(strbuf_view(str_symbol_path)));
        }

        prev_depth = it.depth;
    }

    // Build query string.

    strbuf_cat(
        str_query,
        cstr_SL("py woy_query_symbol_multiple(["),
        strbuf_view(str_list),
        cstr_SL("], True)\n")
    );

    // Send it.

    printfd("1 (%"PRIstr")", PRIstrarg(strbuf_view(str_symbol_path)));
    printfd("2 (%"PRIstr")", PRIstrarg(strbuf_view(str_list)));
    printfd("3 (%"PRIstr")", PRIstrarg(strbuf_view(str_query)));

    int error = IPC_write_cmd(&ctx->ipc_ctx, strbuf_view(str_query));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt, IPC_WAIT_DO_READ_WOY_LOCALS, &ctx->wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(&ctx->ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ctx->ipc_ctx, &ctx->reader, &ctx->cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
}

/*
   The only thing it does is try to keep the same nodes expanded.
 */
void SymbolTree_update(SymbolTree *base, SymbolTree *update) {
    //SymbolTree_Iterator it_base = { 0 };
    //while (SymbolTree_get_next(base, &it_base)) {
        //if (!it_base.item->is_expanded) { continue; }

        //SymbolTree_Iterator it_update = { 0 };

        //// Find matching node.
        //while (SymbolTree_get_next(update, it_update)) {
        //}

    //}

}


void trigger_fileline_refresh(Ctx *ctx) {
    const int FILELINE_REFRESH_TICKS = 3;
    ctx->refresh_fileline_ticks_left = FILELINE_REFRESH_TICKS;
}


void process_update_fileline(Ctx *ctx) {
    if (ctx->refresh_fileline_ticks_left < 0) { return; }
    --ctx->refresh_fileline_ticks_left;
    if (ctx->refresh_fileline_ticks_left > 0) { return; }

    printfd("We're gonna be doing this again ok?");

    //wait_request_get_locals(ctx);
    wait_request_get_file_and_line(ctx);
    update_file_view_from_file_line_query(ctx);
}


#endif // !EVENTS_H_IMPLEMENTATION
