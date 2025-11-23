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
      uint id = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      uint type = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      uint enabled = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      strview_t location = strview_split_line(&src, NULL);
      strview_t file = strview_split_line(&src, NULL);
      uint line_number = strnum_u32(strview_split_line(&src, NULL), 0, STRNUM_DEFAULT);
      strview_split_line(&src, NULL); // control line '---'

      WuiBreakpoint breakpoint;
      WuiBreakpoint_init(&breakpoint);
      breakpoint.id = id;
      breakpoint.type = type;
      breakpoint.enabled = enabled;
      breakpoint.line = line_number;

      tmp = &breakpoint.location;
      strbuf_assign(&tmp, location);
      tmp = &breakpoint.file;
      strbuf_assign(&tmp, file);

      WuiBreakpoint_DynArr_insert(&wui_state->breakpoints, breakpoint);

      printf("id %d type %d enabled %s loc %s file %s line %d\n",
            breakpoint.id, breakpoint.type, PRIbool(breakpoint.enabled),
            breakpoint.location.cstr, breakpoint.file.cstr, line_number);
   }
}



