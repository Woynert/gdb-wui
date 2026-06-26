/*
   gcc ./src/containers/pool_test.c && ./a.out
*/


#include "../standalone/portable_utils.h"

#define POOL__TYPE float
#define POOL__NAMESPACE float_Pool
#include "pool.h"


void print_all(float_Pool *pool) {
    printfd("\nPRINTING ALL:");
    printfd("capacity %d available_count %d", pool->capacity, pool->available_count);
    for (int i = 0; i < pool->capacity; ++i) {
        if (pool->nodes[i].used) {
            printfd("id %d value %f", i, pool->nodes[i].value);
        }
    }
    printf("\n");
}


void test1(void) {
    int err;
    float_Pool pool = { 0 };

    err = float_Pool_create(&pool);
    wassert(err == 0);

    int id;
    float og_item;
    float *item = NULL;

    og_item = 314;
    int id1 = float_Pool_insert(&pool, og_item);
    id = id1;
    wassert(id != -1);
    item = float_Pool_get(&pool, id);
    wassert(item != NULL);
    wassert(*item == og_item);
    printfd("id %d item %f og_item %f", id, *item, og_item);
    print_all(&pool);

    og_item = 500;
    int id2 = float_Pool_insert(&pool, og_item);
    id = id2;
    wassert(id != -1);
    item = float_Pool_get(&pool, id);
    wassert(item != NULL);
    wassert(*item == og_item);
    printfd("id %d item %f og_item %f", id, *item, og_item);
    print_all(&pool);

    og_item = 99999;
    int id3 = float_Pool_insert(&pool, og_item);
    id = id3;
    wassert(id != -1);
    item = float_Pool_get(&pool, id);
    wassert(item != NULL);
    wassert(*item == og_item);
    printfd("id %d item %f og_item %f", id, *item, og_item);
    print_all(&pool);

    // delete

    err = float_Pool_remove(&pool, id2);
    wassert(err == 0);
    print_all(&pool);

    err = float_Pool_remove(&pool, id1);
    wassert(err == 0);
    print_all(&pool);

    og_item = -25;
    id = float_Pool_insert(&pool, og_item);
    wassert(id != -1);
    item = float_Pool_get(&pool, id);
    wassert(item != NULL);
    wassert(*item == og_item);
    printfd("id %d item %f og_item %f", id, *item, og_item);
    print_all(&pool);

    err = float_Pool_remove(&pool, id3);
    wassert(err == 0);
    print_all(&pool);

    for (float i = 100; i < 110; i += 1) {
        id = float_Pool_insert(&pool, i);
        wassert(id != -1);
        item = float_Pool_get(&pool, id);
        wassert(item != NULL);
        wassert(*item == i);
    }
    print_all(&pool);
}


int main(void) {
    test1();
}
