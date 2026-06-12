#include "stdbool.h"
#include "stdio.h"

/*
 * Simple tree data structure.
 *
 * Nodes are stored in a flat array hierarchically. For example the tree:
 *
 * - 1
 *    - 2
 *        - 4
 *    - 3
 *
 * Would be stored as: [1, 2, 4, 3].
 * Inserting 5 under 4 would look like this: [1, 2, 4, 5, 3].
 *
 * Usage:
 *
 * #define TREESI__TYPE <type>
 * #define TREESI__NAMESPACE <custom name> (optional)
 * #include "tree_simple.h"
 * #undef  TREESI__TYPE
 * #undef  TREESI__NAMESPACE
*/

/* User didn't specify type, using default. */
#ifndef TREESI__TYPE
#define TREESI__TYPE float
#endif

/* Token concatenation. */
#define TREESI__TOKCAT_(a, b) a ## b
#define TREESI__TOKCAT(a, b) TREESI__TOKCAT_(a, b)
#ifndef TREESI__NAMESPACE
#define TREESI__NAMESPACE TREESI__TOKCAT(TREESI__TYPE, _TreeSi)
#endif
#define TREESI__PRE(name) TREESI__TOKCAT(TREESI__TOKCAT(TREESI__NAMESPACE, _), name)

typedef unsigned int uint;
typedef struct {
    uint id;
    uint parent_id;
    int depth;
    TREESI__TYPE item;
} TREESI__PRE(Node);

#define DYNA__TYPE TREESI__PRE(Node)
#undef DYNA__PREFIX
#include "da.h"

#define pfx(name) TREESI__PRE(name)
#define TYPE TREESI__TYPE
#define TreeSi TREESI__NAMESPACE

typedef struct {
    pfx(Node_Dyna) nodes;
    uint id_counter;
} TreeSi;

static TreeSi     pfx(create)     (void);
static void       pfx(free)       (TreeSi *tree);
static void       pfx(clear)      (TreeSi *tree);
static bool       pfx(node_exists)(TreeSi *tree, uint node_id);
static int        pfx(create_node)(TreeSi *tree, uint parent_id, uint *out_node_id, TYPE item);
static int        pfx(destroy_node_and_children)(TreeSi *tree, uint node_id);
static void       pfx(print)      (TreeSi *tree);
static pfx(Node) *pfx(_find_node) (TreeSi *tree, uint node_id, int *out_node_index);


static TreeSi pfx(create)(void) {
    TreeSi tree = { 0 };
    tree.id_counter = 1;
    tree.nodes = pfx(Node_Dyna_create)(); /* @note: Can't fail. */
    return tree;
}


static void pfx(free)(TreeSi *tree) {
    tree->id_counter = 0;
    pfx(Node_Dyna_free)(&tree->nodes);
}


static void pfx(clear)(TreeSi *tree) {
    tree->id_counter = 0;
    pfx(Node_Dyna_clear_preserve)(&tree->nodes);
}


/*
   @param[out] out_node_index Optional
   @returns pfx(Node) or NULL
*/
pfx(Node) *pfx(_find_node)(TreeSi *tree, uint node_id, int *out_node_index) {
    for (int i = 0; i < tree->nodes.size; ++i) {
        if (tree->nodes.items[i].id == node_id) {
            if (out_node_index != NULL) {
                *out_node_index = i;
            }
            return &tree->nodes.items[i];
        }
    }
    return NULL;
}


bool pfx(node_exists)(TreeSi *tree, uint node_id) {
    for (int i = 0; i < tree->nodes.size; ++i) {
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
int pfx(create_node)(TreeSi *tree, uint parent_id, uint *out_node_id, TYPE item)
{
    pfx(Node) parent_node = { 0 };
    int parent_index = 0;
    bool has_parent = parent_id != 0;

    if (has_parent) {
        // Confirm parent exists.
        pfx(Node) *tmp = pfx(_find_node)(tree, parent_id, &parent_index);
        if (tmp == NULL) { return -1; }
        parent_node = *tmp;
    }

    pfx(Node) node = {
        .id = tree->id_counter,
        .parent_id = parent_id,
        .depth = has_parent ? parent_node.depth +1 : 0,
        .item = item
    };

    if (has_parent) {
        int error = pfx(Node_Dyna_insert_at_preserve_order)(
                &tree->nodes, parent_index +1, node);
        if (error != 0) { return error; }
    }
    else {
        pfx(Node_Dyna_append)(&tree->nodes, node);
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
int pfx(destroy_node_and_children)(TreeSi *tree, uint node_id) {
    int node_index;
    pfx(Node) *node;

    node = pfx(_find_node)(tree, node_id, &node_index);
    if (node == NULL) { return -1; }

    int parent_depth = node->depth;

    node = NULL;
    int error = pfx(Node_Dyna_pop_at_preserve_order)(&tree->nodes, node_index);
    if (error != 0) { return -2; };

    /* Not very efficient but good enough. Assumes order is hierarchical. */
    while(true) {
        if (node_index < 0 || node_index >= tree->nodes.size) { break; }
        node = &tree->nodes.items[node_index];
        if (node->depth > parent_depth) {
            node = NULL;
            error = pfx(Node_Dyna_pop_at_preserve_order)(&tree->nodes, node_index);
            if (error != 0) { return -3; };
        }
        else break;
    }

    return 0;
}


void pfx(print)(TreeSi *tree) {
    printf("------\n");
    if (tree->nodes.size == 0) {
        printf("[EMPTY TREE]\n");
        return;
    }
    for (int i = 0; i < tree->nodes.size; ++i) {
        pfx(Node) *node = &tree->nodes.items[i];
        for (int k = 0; k < node->depth * 4; ++k) { printf(" "); }
        printf("- %d\n", node->id);
    }
}


#undef TREESI__TOKCAT_
#undef TREESI__TOKCAT
#undef TREESI__PRE
#undef DYNA__TYPE
#undef DYNA__PREFIX
#undef pfx
#undef TYPE
#undef TreeSi
// Don't undef these since they are user generated:
// TREESI__TYPE
// TREESI__NAMESPACE
