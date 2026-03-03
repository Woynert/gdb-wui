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


#define DYN_ARR_TYPE WuiBreakpoint
#include "./containers/da.h"
#undef DYN_ARR_TYPE

#define TREESI__TYPE WuiSymbol
#define TREESI__NAMESPACE SymbolTree
#include "./containers/tree_simple.h"
#undef  TREESI__TYPE
#undef  TREESI__NAMESPACE

typedef struct WuiState {
    WuiBreakpoint_DynArr breakpoints;
    SymbolTree symbol_tree;
} WuiState;

void WuiState_init (WuiState *wui_state) {
    wui_state->breakpoints = WuiBreakpoint_DynArr_create();
    wui_state->symbol_tree = SymbolTree_create();
}

#endif // !WUI_STATE

