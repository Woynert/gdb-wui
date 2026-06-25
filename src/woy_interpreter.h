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
#include "events.h"

#define WOY_INTERPRETER_BUFFER_SIZE 4096

#define WOY_INTERPRETER_MAGIC_SYMBOL "[SYM]"
#define WOY_INTERPRETER_MAGIC_PARENT "[PAR]"
#define WOY_INTERPRETER_MAGIC_PARENT_END "[END]"
#define WOY_INTERPRETER_MAGIC_CHILDREN_INFO "[CHI]"


typedef struct WoyInterp {
    union {
        strbuf_space_t(WOY_INTERPRETER_BUFFER_SIZE) _buffer;
        strbuf_t buffer;
    };
} WoyInterp;


void WoyInterp_init(WoyInterp *woy_interp);
void WoyInterp_reset(WoyInterp *woy_interp);
void WoyInterp_push_line(WoyInterp *woy_interp, strview_t line);
void WoyInterp_interpret_breakpoints(WoyInterp *woy_interp, WuiState *wui_state);
int WoyInterp_interpret_symbols(WoyInterp *woy_interp, WuiSymbolTree *wui_symbol_tree, uint symbol_id);
int WoyInterp_interpret_file_line(WoyInterp *woy_interp, WuiState *wui_state);


#endif // !WOY_INTERPRETER_H
#include "have_lsp.h"
#if (defined WOY_INTERPRETER_H_IMPLEMENTATION || defined HAVE_LSP) && !defined WOY_INTERPRETER_H_DONE
#define WOY_INTERPRETER_H_DONE
/*
 * █ █▀▄▀█ █▀█ █   █▀▀ █▀▄▀█ █▀▀ █▄ █ ▀█▀ ▄▀█ ▀█▀ █ █▀█ █▄ █
 * █ █ ▀ █ █▀▀ █▄▄ ██▄ █ ▀ █ ██▄ █ ▀█  █  █▀█  █  █ █▄█ █ ▀█
 */


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

   WuiBreakpoint_Dyna_clear_preserving(&wui_state->breakpoints);

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

      WuiBreakpoint_Dyna_append(&wui_state->breakpoints, breakpoint);

      printf("id %d type %d enabled %s loc %s file %s line %d\n",
            breakpoint.id, breakpoint.type, PRIbool(breakpoint.enabled),
            breakpoint.location.cstr, breakpoint.file.cstr, breakpoint.line);
   }
}




/*
  @brief Builds the symbol tree
  @param symbol_id. If 0 will overwrite the entire tree. (Used for locals)
                    If not 0 will get symbol, clear children and append new ones.
  @returns error.
*/
int WoyInterp_interpret_symbols(
   WoyInterp *woy_interp, WuiSymbolTree *wui_symbol_tree, uint symbol_id
) {
   const strview_t INPUT = strbuf_view2(&woy_interp->buffer);
   strview_t input = INPUT;
   strview_t view;

   view = strview_split_first_delim(&input, ",", false);
   if (!strview_equal(view, cstr_SL("^done"))) {
      printfd("Error");
      return 0;
   }

   // Split 'variables=['
   strview_split_first_delim(&input, "[", false); 
   // Strip ']\n'
   input.size -= 2; 

   printfd("We are left with this {%"PRIstr"}", PRIstrarg(input));

   for (;;) {
      view = strview_split_first_delim(&input, "}", true);
      if (input.size <= 0) { break; }
      view = strview_trim(view, cstr_SL(",{}"));

      WuiSymbol symbol;
      WuiSymbol_init(&symbol);

      for (;;) {
         if (view.size <= 0) { break; }

         strview_t key_value_pair = strview_split_first_delim(&view, ",", true);
         strview_t key = strview_split_first_delim(&key_value_pair, "=", false);
         strview_t value = strview_trim(key_value_pair, "\"");

         if (strview_equal(key, cstr_SL("name"))) {
            int view_id = Stringpool_append(&wui_symbol_tree->strpool, value);
            symbol.str_id_name = view_id;
         } else if (strview_equal(key, cstr_SL("value"))) {
            int view_id = Stringpool_append(&wui_symbol_tree->strpool, value);
            symbol.str_id_value = view_id;
         }

         printfd("Symbol [%"PRIstr" = %"PRIstr"]", PRIstrarg(key), PRIstrarg(value));
      }

      if (symbol.str_id_name <= 0 ||
          symbol.str_id_value <= 0
      ) { continue; }

      // Insert.

      uint out_node_id;
      int err = SymbolTree_create_node(&wui_symbol_tree->tree, symbol_id, &out_node_id, symbol);
      if (err != 0) {
         printfd("ERROR: Couldn't insert node.");
         continue;
      }
   }

   return 0;

   /*
      Reference input:
      (gdb) interpreter-exec mi "-stack-list-variables --no-frame-filters 1"
      ^done,variables=[{name="gs",arg="1",value="0x7fffffff07a0"},{name="world",value="<optimized out>"},{name="shader",value="{id = <optimized out>, locs = <optimized out>}"},{name="ambientColor",value="{r = <optimized out>, g = <optimized out>, b = <optimized out>, a = <optimized out>}"},{name="ambientColorNormalized",value="{x = <optimized out>, y = <optimized out>, z = <optimized out>}"},{name="light",value="{enabled = <optimized out>, type = <optimized out>, position = {x = <optimized out>, y = <optimized out>, z = <optimized out>}, target = {x = <optimized out>, y = <optimized out>, z = <optimized out>}, color = {r = <optimized out>, g = <optimized out>, b = <optimized out>, a = <optimized out>}, intensity = <optimized out>, attenuation_factor = <optimized out>, enabled_loc = <optimized out>, type_loc = <optimized out>, position_loc = <optimized out>, target_loc = <optimized out>, color_loc = <optimized out>, intensity_loc = <optimized out>, attenuation_factor_loc = <optimized out>}"},{name="error",value="<optimized out>"}]
      (gdb) interpreter-exec mi "-stack-list-variables --no-frame-filters 2"
      ^done,variables=[{name="gs",arg="1",type="GameState *",value="0x7fffffff07a0"},{name="world",type="World *",value="<optimized out>"},{name="shader",type="Shader"},{name="ambientColor",type="Color"},{name="ambientColorNormalized",type="Vector3"},{name="light",type="Light"},{name="error",type="int",value="<optimized out>"}]
   */
}


#if 0
int WoyInterp_interpret_symbols_old_DOESNT_WORK(
   WoyInterp *woy_interp, SymbolTree *symbol_tree, uint symbol_id
) {
   strbuf_t *tmp = &woy_interp->buffer;
   strview_t src = strbuf_view(&tmp);
   strview_t line;

   line = strview_split_line(&src, NULL);
   uint success = strnum_u32(line, 0, STRNUM_DEFAULT);
   if (success == 0) { return -1; }

   if (symbol_id == 0) {
      SymbolTree_clear(symbol_tree);
   }
   else {
      int err = SymbolTree_destroy_children(symbol_tree, symbol_id);
      if (err != 0) {
         return -3;
      }
   }

   bool use_references     = false;
   bool next_one_is_parent = false;
   bool have_parent        = false;
   uint parent_symbol_id;

   for (;;) {
      printfd("-");
      line = strview_split_line(&src, NULL);

      if (strview_compare(line, cstr(WOY_INTERPRETER_MAGIC_CHILDREN_INFO)) == 0) {
         use_references = true;
         continue;
      }
      if (strview_compare(line, cstr(WOY_INTERPRETER_MAGIC_PARENT)) == 0) {
         next_one_is_parent = true;
         have_parent = false;
         printfd("%"PRIstr, PRIstrarg(line));
         continue;
      } else if (strview_compare(line, cstr(WOY_INTERPRETER_MAGIC_PARENT_END)) == 0) {
         next_one_is_parent = false;
         have_parent = false;
         printfd("%"PRIstr, PRIstrarg(line));
         continue;
      } else if (strview_compare(line, cstr(WOY_INTERPRETER_MAGIC_SYMBOL)) != 0) {
         printfd("%"PRIstr, PRIstrarg(line));
         break;
      }


      // Read symbol

      WuiSymbol symbol;
      WuiSymbol_init(&symbol);

      symbol.basic_type = strnum_i32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);

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


      if (use_references && next_one_is_parent) {

         // Find the correct parent to add it to.

         SymbolTree_Iterator out_it_read_only = { 0 };
         int err = SymbolTree_find_node_by_full_path(
                     symbol_tree, strbuf_view2(&symbol.symbol_name), &out_it_read_only);
         if (err == 0) {
            have_parent = true;
            next_one_is_parent = false;
            parent_symbol_id = out_it_read_only.node_id;
            out_it_read_only.item->is_expanded = true;
            continue;
         }
      }

      /* @note: This could be removed if we remove the case where the [PAR] tag appends
         to the parent instead of a reference. */
      if (next_one_is_parent) { symbol.is_expanded = true; } 

      uint add_to_symbol_id = have_parent ? parent_symbol_id : symbol_id;

      uint out_node_id;
      int err = SymbolTree_create_node(symbol_tree, add_to_symbol_id, &out_node_id, symbol);

      if (err != 0) {
         printfd("ERROR: Couldn't insert node.");
         next_one_is_parent = false;
         continue;
      }

      if (next_one_is_parent) {
         next_one_is_parent = false;
         have_parent = true;
         parent_symbol_id = out_node_id;
      }
   }

   return 0;
}
#endif

/* @returns Error. */
int WoyInterp_interpret_file_line(WoyInterp *woy_interp, WuiState *wui_state) {
   strbuf_t *tmp = &woy_interp->buffer;
   strview_t src = strbuf_view(&tmp);
   strview_t line;
   wui_state->has_valid_location = false;

   line = strview_split_line(&src, NULL);
   uint success = strnum_u32(line, 0, STRNUM_DEFAULT);
   if (success == 0) { return -1; }

   line = strview_split_line(&src, NULL);
   if (!strview_is_valid(line)) { return -1; }
   strbuf_assign(&wui_state->curr_file_path, line);

   line = strview_split_line(&src, NULL);
   if (!strview_is_valid(line)) { return -1; }
   int line_number = strnum_i32(line, 0, STRNUM_DEFAULT);
   wui_state->curr_line = line_number;

   // DELME
   printf("FILE_LINE GOT: %"PRIstr" : %d\n",
      PRIstrarg(strbuf_view(&wui_state->curr_file_path)), wui_state->curr_line);
   // DELME

   wui_state->has_valid_location = true;
   return 0;
}

#endif // !WOY_INTERPRETER_H_IMPLEMENTATION
