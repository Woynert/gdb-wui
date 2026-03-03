#ifndef WOY_INTERPRETER_H
#define WOY_INTERPRETER_H

#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"
#include "strnum.h"
#include "stdio.h"
#include "sys/types.h"
#include "wui_state.h"
#include "portable_utils.h"

#define WOY_INTERPRETER_BUFFER_SIZE 4096


typedef struct WoyInterp {
    union {
        strbuf_space_t(WOY_INTERPRETER_BUFFER_SIZE) _buffer;
        strbuf_t buffer;
    };
} WoyInterp;


void WoyInterp_init(WoyInterp *woy_interp) {
    *woy_interp = (WoyInterp) { 0 };
    STRBUF_STATIC_INIT2(WOY_INTERPRETER_BUFFER_SIZE, woy_interp->_buffer);
}


/*
   Clears internal buffer
*/
void WoyInterp_reset(WoyInterp *woy_interp) {
   strbuf_t *tmp = &woy_interp->buffer;
   strbuf_assign(&tmp, cstr(""));
}


/*
   Appends a line in the buffer
*/
void WoyInterp_push_line(WoyInterp *woy_interp, strview_t line) {
   strbuf_t *tmp = &woy_interp->buffer;
   // TODO: handle out of space
   strbuf_append(&tmp, line);
   strbuf_append(&tmp, "\n");
}


/*
   Check ./gdb_woy_api.py for woy_get_breakpoints() API
*/
void WoyInterp_interpret_breakpoints(WoyInterp *woy_interp, WuiState *wui_state) {
   strbuf_t *tmp = &woy_interp->buffer;
   strview_t src = strbuf_view(&tmp);
   strview_t line;

   line = strview_split_line(&src, NULL);
   uint cases = strnum_u32(line, 0, STRNUM_DEFAULT);
   printf("\ncases %d\n", cases);

   WuiBreakpoint_DynArr_clear_preserving_capacity(&wui_state->breakpoints);

   for (uint i = 0; i < cases; ++i) {
      WuiBreakpoint breakpoint;
      WuiBreakpoint_init(&breakpoint);

      breakpoint.id = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      breakpoint.type = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      breakpoint.enabled = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      strview_t location = strview_split_line(&src, NULL);
      strview_t file = strview_split_line(&src, NULL);
      breakpoint.line = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      strview_split_line(&src, NULL); // control line '---'

      tmp = &breakpoint.location;
      strbuf_assign(&tmp, location);
      tmp = &breakpoint.file;
      strbuf_assign(&tmp, file);

      WuiBreakpoint_DynArr_insert(&wui_state->breakpoints, breakpoint);

      printf("id %d type %d enabled %s loc %s file %s line %d\n",
            breakpoint.id, breakpoint.type, PRIbool(breakpoint.enabled),
            breakpoint.location.cstr, breakpoint.file.cstr, breakpoint.line);
   }
}


/*
  @brief Builds the symbol tree
  @param symbol_id. If 0 will overwrite the entire tree. (Used for locals)
                    Otherwise will append symbols as child of symbol_id.
*/
int WoyInterp_interpret_symbols(
   WoyInterp *woy_interp, WuiState *wui_state, uint symbol_id
) {
   strbuf_t *tmp = &woy_interp->buffer;
   strview_t src = strbuf_view(&tmp);
   strview_t line;

   line = strview_split_line(&src, NULL);
   uint success = strnum_u32(line, 0, STRNUM_DEFAULT);
   if (!success) { return -1; }

   if (symbol_id != 0 && !SymbolTree_node_exists(&wui_state->symbol_tree, symbol_id)) {
      return -2;
   }

   if (symbol_id == 0) {
      SymbolTree_clear(&wui_state->symbol_tree);
   }

   while (true) {
      line = strview_split_line(&src, NULL);
      if (strview_compare(line, cstr("vvv")) != 0) {
         break;
      }

      WuiSymbol symbol;
      WuiSymbol_init(&symbol);

      symbol.basic_type = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);

      line = strview_split_line(&src, NULL);
      tmp = &symbol.type_name;
      strbuf_assign(&tmp, line);

      line = strview_split_line(&src, NULL);
      tmp = &symbol.symbol_name;
      strbuf_assign(&tmp, line);

      line = strview_split_line(&src, NULL);
      tmp = &symbol.value;
      strbuf_assign(&tmp, line);

      line = strview_split_line(&src, NULL);
      tmp = &symbol.address;
      strbuf_assign(&tmp, line);

      SymbolTree_create_node(&wui_state->symbol_tree, symbol_id, NULL, symbol);

      //printf("Symbol: type %d, type_name %s, symbol_name %s, value %s, address %s\n",
            //symbol.basic_type, symbol.type_name.cstr, symbol.symbol_name.cstr,
            //symbol.value.cstr, symbol.address.cstr);
   }
   //printf("PRINTING SYMBOLS BELOW VVV\n");
   // print tree contents
   //for (uint i = 0; i < wui_state->symbol_tree.nodes.size; ++i) {
      //WuiSymbol symbol = wui_state->symbol_tree.nodes.items[i].item;
      //for (uint k = 0; k < 4 * wui_state->symbol_tree.nodes.items[i].depth;
      //     ++k) { printf(" "); }
      //printf("(TREE_ID %d parent %d) Symbol: type %d, type_name %s, symbol_name %s, value %s, address %s\n",
            //wui_state->symbol_tree.nodes.items[i].id,
            //wui_state->symbol_tree.nodes.items[i].parent_id,
            //symbol.basic_type, symbol.type_name.cstr, symbol.symbol_name.cstr,
            //symbol.value.cstr, symbol.address.cstr);
   //}

   return 0;
}

#endif // !WOY_INTERPRETER_H
