#ifndef WUI_STATE
#define WUI_STATE

#include "stdbool.h"
#include "sys/types.h"
#include "strbuf.h"
#include "strbuf_extra.h"
#include "containers/stringpool.h"

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

/* See gdb_woy_api.py. */
#define SYMBOL_TYPE_PTR    1
#define SYMBOL_TYPE_ARRAY  2
#define SYMBOL_TYPE_STRUCT 3
#define SYMBOL_TYPE_FUNC   7

#define WUI_SYMBOL_VALUE_STR_SIZE 100

typedef struct WuiSymbol {
    bool is_expanded; /* For queries. */
    int basic_type;
    int str_id_name;
    int str_id_value;
    int str_id_type_name;
    int str_id_address;
} WuiSymbol;

void WuiSymbol_init (WuiSymbol *wui_symbol) {
    *wui_symbol = (WuiSymbol) { 0 };
}

typedef enum EVENT {
    EVENT_SYMBOL_QUERY,
    EVENT_MAX,
} EVENT;

typedef struct EventSymbolQuery {
    uint symbol_node_id;
} EventSymbolQuery;

typedef struct Event {
    EVENT type;
    union {
        EventSymbolQuery event_symbol_query;
    };
} Event;

#define DYNA__TYPE WuiBreakpoint
#include "./containers/da.h"

#define TREESI__TYPE WuiSymbol
#define TREESI__NAMESPACE SymbolTree
#include "./containers/tree_simple.h"

#define DYNA__TYPE Event
#define DYNA__NAMESPACE Event_da
#include "containers/da.h"

typedef struct WuiSymbolTree {
    SymbolTree tree;
    Stringpool strpool;
} WuiSymbolTree;

typedef struct WuiState {
    WuiBreakpoint_Dyna breakpoints;

    WuiSymbolTree locals;
    WuiSymbolTree tmp_tree;

    bool has_valid_location; /* Guards current file path and line. */
    strbuf_t *curr_file_path; /* Absolute path. */
    int curr_line;
    strbuf_t *file_contents_tmp_buf;

    Event_da events;
} WuiState;

void WuiState_init (WuiState *wui_state) {
    *wui_state = (WuiState) { 0 };
    wui_state->breakpoints = WuiBreakpoint_Dyna_create();
    wui_state->curr_file_path = strbuf_create(0, NULL);
    wui_state->file_contents_tmp_buf = strbuf_create(0, NULL);
    wui_state->events = Event_da_create();

    wui_state->locals.tree = SymbolTree_create();
    Stringpool_create(&wui_state->locals.strpool);

    wui_state->tmp_tree.tree = SymbolTree_create();
    Stringpool_create(&wui_state->tmp_tree.strpool);
}

void WuiState_free (WuiState *wui_state) {
    WuiBreakpoint_Dyna_free(&wui_state->breakpoints);
    strbuf_destroy(&wui_state->curr_file_path);
    strbuf_destroy(&wui_state->file_contents_tmp_buf);
    Event_da_free(&wui_state->events);
    *wui_state = (WuiState) { 0 };

    SymbolTree_free(&wui_state->locals.tree);
    Stringpool_destroy(&wui_state->locals.strpool);

    SymbolTree_free(&wui_state->tmp_tree.tree);
    Stringpool_destroy(&wui_state->tmp_tree.strpool);
}

#endif // !WUI_STATE

