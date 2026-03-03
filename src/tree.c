#include "stdio.h"
#include "src/portable_utils.h"
#define OK 0

typedef struct Mimo {
    int mimo_id;
    char name[30];
} Mimo;

#define TREESI__TYPE Mimo
#define TREESI__NAMESPACE MimoTree
#include "./containers/tree_simple.h"
#undef  TREESI__TYPE
#undef  TREESI__NAMESPACE


void print_my_tree(MimoTree *tree) {
    for (uint i = 0; i < tree->nodes.size; ++i) {
        MimoTree_Node *node = &tree->nodes.items[i];
        for (uint k = 0; k < 4 * node->depth; ++k) { printf(" "); };
        printf("%d:%s\n", node->id, node->item.name);
    }
}

void test1(MimoTree *tree) {
    int error;
    uint node_id;
    error = MimoTree_create_node(tree, 0, &node_id, (Mimo){0, "Mimo100"});
    uint root_one = node_id;
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, &node_id, (Mimo){0, "Mimo101"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, NULL, (Mimo){0, "MimoA1"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, NULL, (Mimo){0, "MimoA2"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    uint second_branch;
    error = MimoTree_create_node(tree, 0, &second_branch, (Mimo){0, "Mimo102"});
    uint root_two = second_branch;
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, &node_id, (Mimo){0, "Mimo103"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, &node_id, (Mimo){0, "Mimo104"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, &second_branch, (Mimo){0, "Mimo105"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, NULL, (Mimo){0, "MimoB1"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, &second_branch, (Mimo){0, "Mimo106"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, NULL, (Mimo){0, "MimoB2"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    print_my_tree(tree);

    error = MimoTree_destroy_node_and_children(tree, node_id);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_destroy_node_and_children(tree, root_one);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, &second_branch, (Mimo){0, "Mimo107"});
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_destroy_node_and_children(tree, root_two);
    ASSERT(error == OK);
    MimoTree_print(tree);
}


int main(void) {

    MimoTree tree = MimoTree_create();

    printf("========================================\n");
    test1(&tree);
    printf("========================================\n");
    test1(&tree);

    printf("Hello there\n");
    return 0;
}
