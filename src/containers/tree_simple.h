#include "stdbool.h"
#include "stdio.h"

/*
 * Usage:
 *
 * #define TREESI__TYPE <type>
 * #define TREESI__NAMESPACE <custom name> (optional)
 * #include "tree_simple.h"
 * #undef  TREESI__TYPE
 * #undef  TREESI__NAMESPACE
 *
 * Examples:
 *
 * #define TREESI__TYPE float
 * #include "tree_simple.h"
 * float_TreeSi tree = float_TreeSi_create();
 *
 * #define TREESI__TYPE int
 * #define TREESI__NAMESPACE IntTree
 * #include "tree_simple.h"
 * IntTree tree = IntTree_create();
*/

/* user didn't specify type, using default */
#ifndef TREESI__TYPE
#define TREESI__TYPE uint
#endif

/* token concatenation */
#define TREESI__TOKCAT_(a, b) a ## b
#define TREESI__TOKCAT(a, b) TREESI__TOKCAT_(a, b)
#ifndef TREESI__NAMESPACE
#define TREESI__NAMESPACE TREESI__TOKCAT(TREESI__TYPE, _TreeSi)
#endif
#define TREESI__PRE(name) TREESI__TOKCAT(TREESI__TOKCAT(TREESI__NAMESPACE, _), name)
/* use TREESI__PRE as namespace */

typedef unsigned int uint;
typedef struct {
    uint id;
    uint parent_id;
    uint depth;
    TREESI__TYPE item;
} TREESI__PRE(Node);

#define DYN_ARR_TYPE TREESI__PRE(Node)
#undef DYN_ARR_PREFIX
#include "da.h"

#define PRE(name) TREESI__PRE(name)
#define TYPE TREESI__TYPE
#define TreeSi TREESI__NAMESPACE

typedef struct {
    PRE(Node_DynArr) nodes;
    uint id_counter;
} TreeSi;

static TreeSi PRE(create)(void) { // TODO: memory managment
    TreeSi tree = { 0 };
    tree.id_counter = 1;
    tree.nodes = PRE(Node_DynArr_create)(); // TODO: check error
    return tree;
}

static void PRE(free)(TreeSi *tree) {
    tree->id_counter = 0;
    PRE(Node_DynArr_free)(&tree->nodes);
}

static void PRE(clear)(TreeSi *tree) {
    tree->id_counter = 0;
    PRE(Node_DynArr_clear_preserving_capacity)(&tree->nodes);
}

/*
   @param[out] out_node_index Optional
   @returns PRE(Node) or NULL
*/
PRE(Node)* PRE(_find_node)(TreeSi *tree, uint node_id, uint *out_node_index) {
    for (uint i = 0; i < tree->nodes.size; ++i) {
        if (tree->nodes.items[i].id == node_id) {
            if (out_node_index != NULL) {
                *out_node_index = i;
            }
            return &tree->nodes.items[i];
        }
    }
    return NULL;
}

bool PRE(node_exists)(TreeSi *tree, uint node_id) {
    for (uint i = 0; i < tree->nodes.size; ++i) {
        if (tree->nodes.items[i].id == node_id) {
            return true;
        }
    }
    return false;
}

/*
   @param parent_id. If 0 node will be added to root.
   @param[out] out_node_id Optional
   @returns error
*/
int PRE(create_node)(TreeSi *tree, uint parent_id, uint *out_node_id, TYPE item)
{
    PRE(Node) parent_node = { 0 };
    uint parent_index = 0;
    bool has_parent = parent_id != 0;

    if (has_parent) {
        // confirm parent exists
        PRE(Node) *tmp = PRE(_find_node)(tree, parent_id, &parent_index);
        if (tmp == NULL) { return -1; }
        parent_node = *tmp;
    }

    PRE(Node) node = {
        .id = tree->id_counter,
        .parent_id = parent_id,
        .depth = has_parent ? parent_node.depth +1 : 0,
        .item = item
    };

    if (has_parent) {
        int error = PRE(Node_DynArr_insert_and_memmove_after)(
                &tree->nodes, parent_index, node);
        if (error != OK) { return error; }
    }
    else {
        PRE(Node_DynArr_insert)(&tree->nodes, node);
    }

    if (out_node_id != NULL) {
        *out_node_id = tree->id_counter;
    }
    ++tree->id_counter;
    return 0;
}

/*
   @returns error
*/
int PRE(destroy_node_and_children)(TreeSi *tree, uint node_id) {
    uint node_index;
    PRE(Node) *node;

    node = PRE(_find_node)(tree, node_id, &node_index);
    if (node == NULL) { return -1; }

    uint parent_depth = node->depth;

    node = NULL;
    int error = PRE(Node_DynArr_pop_and_memmove_at)(&tree->nodes, node_index);
    if (error != OK) { return -2; };

    // Not very efficient but good enough.
    while(true) {
        if (node_index >= tree->nodes.size) {
            break;
        }
        node = &tree->nodes.items[node_index];
        if (node->depth > parent_depth) {
            node = NULL;
            error = PRE(Node_DynArr_pop_and_memmove_at)(&tree->nodes, node_index);
            if (error != OK) { return -3; };
        }
        else break;
    }

    return OK;
}


//typedef struct {
    //uint node_idx;
    //PRE(Node)* node;
//} TreeIterator;


void PRE(print)(TreeSi *tree) {
    printf("------\n");
    if (tree->nodes.size == 0) {
        printf("[EMPTY TREE]\n");
        return;
    }
    for (uint i = 0; i < tree->nodes.size; ++i) {
        PRE(Node) *node = &tree->nodes.items[i];
        for (uint k = 0; k < node->depth * 4; ++k) { printf(" "); }
        printf("- %d\n", node->id);
    }
}


// don't undef: user generated
// TREESI__TYPE
// TREESI__NAMESPACE
#undef TREESI__TOKCAT_
#undef TREESI__TOKCAT
#undef TREESI__PRE
#undef DYN_ARR_TYPE
#undef DYN_ARR_PREFIX
#undef DA_PRE
#undef PRE
#undef TYPE
#undef TreeSi
