#include "stdbool.h"
#include "stdio.h"

/*

   Simple tree data structure.

   * Node id 0 is reserved for root.
   * Root is not a Node.
   * Nodes with parent_id 0 are children of root.
   * Nodes are stored in a flat array hierarchically. For example the tree:

   - 1
      - 2
          - 4
      - 3

   Would be stored as: [1, 2, 4, 3].
   Inserting 5 under 4 would look like this: [1, 2, 4, 5, 3].

   * Usage:

   #define TREESI__TYPE <type>
   #define TREESI__NAMESPACE <custom name> (optional)
   #include "tree_simple.h"

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
#include "da.h"

#define pfx(name) TREESI__PRE(name)
#define TYPE TREESI__TYPE
#define TreeSi TREESI__NAMESPACE

typedef struct {
    pfx(Node_Dyna) nodes;
    uint id_counter;
} TreeSi;

typedef struct {
    int __next_index;
    int depth;
    uint node_id;
    TYPE *item;
} pfx(Iterator);

static TreeSi     pfx(create)      (void);
static void       pfx(free)        (TreeSi *tree);
static void       pfx(clear)       (TreeSi *tree);
static void       pfx(_reset)      (TreeSi *tree);
static int        pfx(create_node) (TreeSi *tree, uint parent_id, uint *out_node_id, TYPE item);
static bool       pfx(get_next)    (const TreeSi *tree, pfx(Iterator) *it);
static int        pfx(get_parent)  (const TreeSi *tree, uint node_id, uint *out_parent_id);
static bool       pfx(node_exists) (const TreeSi *tree, uint node_id);
static int        pfx(destroy_children)(TreeSi *tree, uint node_id);
static int        pfx(destroy_node_and_children)(TreeSi *tree, uint node_id);
static int        pfx(_destroy_node_and_children)(TreeSi *tree, uint node_id, bool destroy_self);
static void       pfx(print)       (const TreeSi *tree);
static TYPE      *pfx(get)         (const TreeSi *tree, uint node_id);
static pfx(Node) *pfx(_find_node)  (const TreeSi *tree, uint node_id, int *out_node_index);


static void pfx(_reset)(TreeSi *tree) {
    tree->id_counter = 1;
}


static TreeSi pfx(create)(void) {
    TreeSi tree = { 0 };
    tree.nodes = pfx(Node_Dyna_create)(); /* @note: Can't fail. */
    pfx(_reset)(&tree);
    return tree;
}


static void pfx(free)(TreeSi *tree) {
    pfx(Node_Dyna_free)(&tree->nodes);
    *tree = (TreeSi) { 0 };
    pfx(_reset)(tree);
}


static void pfx(clear)(TreeSi *tree) {
    pfx(Node_Dyna_clear_preserving)(&tree->nodes);
    pfx(_reset)(tree);
}


/*
   @returns pfx(Node) or NULL
*/
static TYPE *pfx(get)(const TreeSi *tree, uint node_id) {
    for (int i = 0; i < tree->nodes.size; ++i) {
        if (tree->nodes.items[i].id == node_id) {
            return &tree->nodes.items[i].item;
        }
    }
    return NULL;
}


/*
   @param[out] out_node_index Optional
   @returns pfx(Node) or NULL
*/
static pfx(Node) *pfx(_find_node)(const TreeSi *tree, uint node_id, int *out_node_index) {
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


static bool pfx(node_exists)(const TreeSi *tree, uint node_id) {
    for (int i = 0; i < tree->nodes.size; ++i) {
        if (tree->nodes.items[i].id == node_id) {
            return true;
        }
    }
    return false;
}

/* 
   @param[out] out_parent_id Node parent id.
   @returns error
*/
static int pfx(get_parent) (const TreeSi *tree, uint node_id, uint *out_parent_id) {
    if (node_id == 0) { return -1; } /* Root has no parent. */
    int depth = 0;
    bool found = false;

    for (int i = tree->nodes.size-1; i > -1; --i) {
        pfx(Node) *node = &tree->nodes.items[i];
        if (!found) {
            if (node->id == node_id) {
                found = true;
                depth = node->depth;
                if (depth == 0) { return -1; } /* Root level. */
            }
        } else if (node->depth < depth) {
            *out_parent_id = node->id;
            return 0;
        }
    }

    return -1;
}

/*
   @param parent_id. If 0 node will be added to root.
   @param[out] out_node_id Optional
   @returns error
*/
static int pfx(create_node)(TreeSi *tree, uint parent_id, uint *out_node_id, TYPE item)
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
static int pfx(destroy_children)(TreeSi *tree, uint node_id) {
    return pfx(_destroy_node_and_children)(tree, node_id, false);
}


/*
   @returns error
*/
static int pfx(destroy_node_and_children)(TreeSi *tree, uint node_id) {
    return pfx(_destroy_node_and_children)(tree, node_id, true);
}


/*
   @returns error
*/
static int pfx(_destroy_node_and_children)(TreeSi *tree, uint node_id, bool destroy_self) {
    int err;
    int node_index;
    pfx(Node) *node;

    node = pfx(_find_node)(tree, node_id, &node_index);
    if (node == NULL) { return -1; }

    int parent_depth = node->depth;
    node = NULL;

    if (destroy_self) {
        err = pfx(Node_Dyna_pop_at_preserve_order)(&tree->nodes, node_index);
        if (err != 0) { return -2; };
    } else {
        ++node_index;
    }

    /* Not very efficient but good enough. Assumes order is hierarchical. */
    while(true) {
        if (node_index < 0 || node_index >= tree->nodes.size) { break; }
        node = &tree->nodes.items[node_index];
        if (node->depth > parent_depth) {
            node = NULL;
            err = pfx(Node_Dyna_pop_at_preserve_order)(&tree->nodes, node_index);
            if (err != 0) { return -3; };
        }
        else break;
    }

    return 0;
}


/// @note To create and Iterator just zero initialize one: Iterator it = { 0 };
/// @note Modifying the tree while iterating is UB.
/// @param[out] it Iterator.
/// @retval true  Can continue iterating.
/// @retval false End reached.
static bool pfx(get_next) (const TreeSi *tree, pfx(Iterator) *it) {
    if (it->__next_index < 0 || it->__next_index >= tree->nodes.size) {
        it->item = NULL;
        return false;
    }

    it->item = &tree->nodes.items[it->__next_index].item;
    it->node_id = tree->nodes.items[it->__next_index].id;
    it->depth = tree->nodes.items[it->__next_index].depth;
    ++it->__next_index;
    return true;
}


void pfx(print)(const TreeSi *tree) {
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
#undef pfx
#undef TYPE
#undef TreeSi
#undef TREESI__TYPE
#undef TREESI__NAMESPACE
