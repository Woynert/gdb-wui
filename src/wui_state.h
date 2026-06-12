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


#define WUI_SYMBOL_VALUE_STR_SIZE 100

typedef struct WuiSymbol {
    uint basic_type;
    union {
        strbuf_space_t(WUI_SYMBOL_VALUE_STR_SIZE) _type_name;
        strbuf_t type_name;
    };
    union {
        strbuf_space_t(WUI_SYMBOL_VALUE_STR_SIZE) _symbol_name;
        strbuf_t symbol_name;
    };
    union {
        strbuf_space_t(WUI_SYMBOL_VALUE_STR_SIZE) _value;
        strbuf_t value;
    };
    union {
        strbuf_space_t(WUI_SYMBOL_VALUE_STR_SIZE) _address;
        strbuf_t address;
    };
} WuiSymbol;

void WuiSymbol_init (WuiSymbol *wui_symbol) {
    wui_symbol->basic_type = 0;
    STRBUF_STATIC_INIT2(WUI_SYMBOL_VALUE_STR_SIZE, wui_symbol->_type_name);
    STRBUF_STATIC_INIT2(WUI_SYMBOL_VALUE_STR_SIZE, wui_symbol->_symbol_name);
    STRBUF_STATIC_INIT2(WUI_SYMBOL_VALUE_STR_SIZE, wui_symbol->_value);
    STRBUF_STATIC_INIT2(WUI_SYMBOL_VALUE_STR_SIZE, wui_symbol->_address);
}


#define DYNA__TYPE WuiBreakpoint
#include "./containers/da.h"
#undef DYNA__TYPE

#define TREESI__TYPE WuiSymbol
#define TREESI__NAMESPACE SymbolTree
#include "./containers/tree_simple.h"
#undef  TREESI__TYPE
#undef  TREESI__NAMESPACE

typedef struct WuiState {
    WuiBreakpoint_Dyna breakpoints;
    SymbolTree symbol_tree;
    bool has_valid_location; /* Guards current file path and line. */
    strbuf_t *curr_file_path; /* Absolute path. */
    int curr_line;
} WuiState;

void WuiState_init (WuiState *wui_state) {
    *wui_state = (WuiState) { 0 };
    wui_state->breakpoints = WuiBreakpoint_Dyna_create();
    wui_state->symbol_tree = SymbolTree_create();
    wui_state->curr_file_path = strbuf_create(0, NULL);
}

void WuiState_free (WuiState *wui_state) {
    WuiBreakpoint_Dyna_free(&wui_state->breakpoints);
    SymbolTree_free(&wui_state->symbol_tree);
    strbuf_destroy(&wui_state->curr_file_path);
}

#endif // !WUI_STATE

