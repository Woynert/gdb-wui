#include "stdio.h"
#include "src/portable_utils.h"
#define OK 0

typedef struct Mimo {
    int mimo_id;
} Mimo;

#define TREESI__TYPE Mimo
#define TREESI__NAMESPACE MimoTree
#include "./containers/tree_simple.h"
#undef  TREESI__TYPE
#undef  TREESI__PREFIX


void test1(MimoTree *tree) {
    int error;
    uint node_id;
    error = MimoTree_create_node(tree, 0, &node_id);
    uint root_one = node_id;
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, &node_id);
    ASSERT(error == OK);
    MimoTree_print(tree);

    uint second_branch;
    error = MimoTree_create_node(tree, 0, &second_branch);
    uint root_two = second_branch;
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, &node_id);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, node_id, &node_id);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, &second_branch);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, &second_branch);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_destroy_node_and_children(tree, node_id);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_destroy_node_and_children(tree, root_one);
    ASSERT(error == OK);
    MimoTree_print(tree);

    error = MimoTree_create_node(tree, second_branch, &second_branch);
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
