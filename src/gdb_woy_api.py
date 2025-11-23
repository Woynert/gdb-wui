from typing import Set, List


def _woy_func_exists(func_name: str) -> bool:
    return func_name in globals() and callable(globals()[func_name])


def _woy_print_gdb_value(value: gdb.Value, name: str):

    basic_type: gdb.Type = gdb.types.get_basic_type(value.type)
    value_str: str = value.format_string(max_elements=10,max_depth=3)[0:100]

    # memory might be out of reach (or garbage)
    if (basic_type.code == gdb.TYPE_CODE_PTR and not value.is_optimized_out):
        try:
            deref_value: gdb.Value = value.dereference()
            deref_value_str = deref_value.format_string(max_elements=10,max_depth=2)[0:100]
            value_str = deref_value_str
        except Exception as e:
            pass

    print("vvv")
    print(basic_type.code)
    print(value.type)
    print(name)
    print(value_str)

    # address
    if (value.address != None):
        print(hex(int(value.address)))
    elif (basic_type.code == gdb.TYPE_CODE_PTR):
        print(hex(int(value)))
    else:
        print(None)


def woy_locals() -> None:
    try:
        curr_frame: gdb.Frame = gdb.selected_frame()
        block: gdb.Block = curr_frame.block() # throws
    except:
        print("0")
        return

    unique_symbols: Set[gdb.Symbol] = set()

    while block and not (block.is_global or block.is_static):
        symbol: gdb.Symbol
        for symbol in block:
            if (symbol.is_argument or symbol.is_variable or symbol.is_constant):
                unique_symbols.add(symbol)
        block = block.superblock

    print("1")
    symbol: gdb.Symbol
    for symbol in unique_symbols:
        value: gdb.Value = symbol.value(curr_frame) # should never throw
        _woy_print_gdb_value(value, symbol.name)


## @param exp_str GDB query expresion string
## @param page (Optional) >1 Pagination for large structs/arrays
##                        0 Default, no pagination, returns all
##
def woy_query_symbol(exp_str: str, page: int = 0) -> None:

    try:
        value: gdb.Value = gdb.parse_and_eval(exp_str)
    except gdb.error:
        print("0")
        return

    basic_type: gdb.Type = gdb.types.get_basic_type(value.type)

    print("1")

    _woy_print_gdb_value(value, exp_str)
    # print("vvv")

    # memory might be out of reach (or garbage)
    if (basic_type.code == gdb.TYPE_CODE_PTR and not value.is_optimized_out):
        try:
            deref_value: gdb.Value = value.dereference()
            deref_value.format_string(max_elements=1,max_depth=1) # to force read
        except Exception as e:
            return

        value = deref_value
        basic_type = gdb.types.get_basic_type(value.type)

    # check for user hook
    hook_func_name = f"woy_hook_{str(value.type)}"
    if _woy_func_exists(hook_func_name):
        # print("Yes the function exists")
        globals()[hook_func_name](value, exp_str)

    elif basic_type.code == gdb.TYPE_CODE_STRUCT or basic_type.code == gdb.TYPE_CODE_UNION:
        key: str
        field: gdb.Field
        for key, field in gdb.types.deep_items(basic_type):
            field_value: gdb.Value = value[key]
            _woy_print_gdb_value(field_value, key)

    elif basic_type.code == gdb.TYPE_CODE_ARRAY:
        range_field: gdb.Field = value.type.fields()[0]
        lower_bound, upper_bound = value.type.range()
        size = upper_bound - lower_bound + 1

        item: gdb.Value
        for i in range(size):
            item = value[i]
            _woy_print_gdb_value(item, f"({exp_str})[{i}]")


## Other breakpoint commands:
## add     -> break filename:linenumber
## disable -> disable 1
## enable  -> enable  1
def woy_get_breakpoints():
    breakpoint_list: List[gdb.Breakpoint] = gdb.breakpoints()

    print(len(breakpoint_list))
    for bp in breakpoint_list:
        print(bp.number)
        print(bp.type)
        print(int(bp.enabled))
        print(bp.location)
        print(bp.locations[0].source[0])
        print(bp.locations[0].source[1])
        print("---")


## THIS MUST BE DEFINED SOMEWHERE ELSE

def woy_hook_EntityInfo_ConMap(value: gdb.Value, name: str):
    # TODO: Handle the case where it's not initialized (garbage values)
    # It will iterate indefinitely

    # iterate all nodes
    for i in range(int(value["_size"])):
        node: gdb.Value = value["_nodes"][i]
        size: int = int(node["_size"])
        if (size <= 0):
            continue
        # iterate all node items
        for k in range(size):
            _woy_print_gdb_value(
                    node["keys"][k], f"{name}._nodes[{i}].keys[{k}]")
            _woy_print_gdb_value(
                    node["values"][k], f"{name}._nodes[{i}].values[{k}]")



"""
Protocol:

-------------------------------------------------------------------
QUERY:
    py woy_locals()
    py woy_query_symbol(expression)
RESPONSE:
1   success: bool              # if false, stop reading

    {
2   vvv                        # control sequence
3   symbol_basic_type: int
4   symbol_type_name: string
5   symbol_name: string
6   symbol_value: char[100]    # always filled
7   symbol_address: char[100]  # always filled
    }

    # Multiple extra fields if symbol_basic_type == STRUCT: Multiple fields
    {
8   vvv
9   symbol_basic_type: int
10  symbol_type_name: string
11  symbol_name: string
12  symbol_value: char[100]
13  symbol_address: char[100]
    }
    # ... More fields

    # Multiple items if symbol_basic_type == ARRAY
    {
8   vvv
9   symbol_basic_type: int     # same as parent
10  symbol_type_name: string   # same as parent
11  symbol_name: string        # contains index e.g: `gs.players[3]`
12  symbol_value: char[100]
13  symbol_address: char[100]
    }
    # ... More fields


-------------------------------------------------------------------
QUERY: py woy_get_breakpoints()
RESPONSE:
1   (int) Breakpoint amount
    {
2   (int) Breakpoint ID
3   (int) Breakpoint type
4   (int) Breakpoint is enabled (0|1)
5   (string) Breakpoint location
6   (string) Breakpoint file
7   (int) Breakpoint line
    }


Symbol basic types:
 1 gdb.TYPE_CODE_PTR
 2 gdb.TYPE_CODE_ARRAY
 3 gdb.TYPE_CODE_STRUCT
 4 gdb.TYPE_CODE_UNION
 5 gdb.TYPE_CODE_ENUM
 6 gdb.TYPE_CODE_FLAGS
 7 gdb.TYPE_CODE_FUNC
 8 gdb.TYPE_CODE_INT
 9 gdb.TYPE_CODE_FLT
10 gdb.TYPE_CODE_VOID
11 gdb.TYPE_CODE_SET
12 gdb.TYPE_CODE_RANGE
13 gdb.TYPE_CODE_STRING
14 gdb.TYPE_CODE_BITSTRING
15 gdb.TYPE_CODE_ERROR
16 gdb.TYPE_CODE_METHOD
17 gdb.TYPE_CODE_METHODPTR
18 gdb.TYPE_CODE_MEMBERPTR
19 gdb.TYPE_CODE_REF
20 gdb.TYPE_CODE_RVALUE_REF
21 gdb.TYPE_CODE_CHAR
22 gdb.TYPE_CODE_BOOL
23 gdb.TYPE_CODE_COMPLEX
24 gdb.TYPE_CODE_TYPEDEF
25 gdb.TYPE_CODE_NAMESPACE
26 gdb.TYPE_CODE_DECFLOAT
27 gdb.TYPE_CODE_INTERNAL_FUNCTION
28 gdb.TYPE_CODE_XMETHOD
29 gdb.TYPE_CODE_FIXED_POINT
30 gdb.TYPE_CODE_NAMESPACE

Breakpoint types:
TBD


"""

