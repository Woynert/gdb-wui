#ifndef WUI_STATE
#define WUI_STATE

#include "stdbool.h"
#include "sys/types.h"
#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"

#define WUI_BREAKPOINT_STR_SIZE 255

typedef struct WuiBreakpoint {
    uint id;
    uint type;
    bool enabled;
    uint line;
    union {
        strbuf_space_t(WUI_BREAKPOINT_STR_SIZE) _location;
        strbuf_t location;
    };
    union {
        strbuf_space_t(WUI_BREAKPOINT_STR_SIZE) _file;
        strbuf_t file;
    };
} WuiBreakpoint;


void WuiBreakpoint_init (WuiBreakpoint *bp) {
    *bp = (WuiBreakpoint) { 0 };
    STRBUF_STATIC_INIT2(WUI_BREAKPOINT_STR_SIZE, bp->_location);
    STRBUF_STATIC_INIT2(WUI_BREAKPOINT_STR_SIZE, bp->_file);
}


#define DYN_ARR_TYPE WuiBreakpoint
#include "./containers/da.h"
#undef DYN_ARR_TYPE


typedef struct WuiState {
    WuiBreakpoint_DynArr breakpoints;
} WuiState;

void WuiState_init (WuiState *wui_state) {
    wui_state->breakpoints = WuiBreakpoint_DynArr_create();
}

#endif // !WUI_STATE

