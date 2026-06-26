/*
   gcc ./src/containers/map_test.c && ./a.out
*/


#include "portable_utils.h"

#define MAP__DEBUG
#define MAP__ENABLE_ASSERT

#define MAP__TYPE uint
#define MAP__NAMESPACE MyMap
#include "map.h"



void test1(void) {
    MyMap map = { 0 };
    MyMap_create(&map);
    MyMap_print_bitmap(&map);
    ASSERT(map.cols == MAP__DEFAULT_COLS);
    ASSERT(map.rows == MAP__DEFAULT_ROWS);

    MyMap_set_pair(&map, -1, 99);
    MyMap_print_bitmap(&map);

    MyMap_set_pair(&map, 20, 99);
    MyMap_print_bitmap(&map);

    MyMap_set_pair(&map, 21, 80);
    MyMap_print_bitmap(&map);

    MyMap_set_pair(&map, 12, 44);
    MyMap_print_bitmap(&map);

    MyMap_set_pair(&map, 14, 23);
    MyMap_print_bitmap(&map);
    ASSERT(map.cols == MAP__DEFAULT_COLS);
    ASSERT(map.rows == MAP__DEFAULT_ROWS);

    MyMap_destroy(&map);
}

void test2(void) {
    MyMap map = { 0 };
    MyMap_create(&map);
    MyMap_print_bitmap(&map);
    ASSERT(map.cols == MAP__DEFAULT_COLS);
    ASSERT(map.rows == MAP__DEFAULT_ROWS);

    int key;
    int err;
    uint og_value;
    uint *value = NULL;
    uint out_value;

    key = -1;
    og_value = 99;
    err = MyMap_set_pair(&map, key, og_value);
    ASSERT(err == 0);
    value = MyMap_get(&map, key);
    ASSERT(value);
    ASSERT(*value == og_value);
    printfd("Got this value mate %d", *value);
    MyMap_print_bitmap(&map);

    key = INT_MAX;
    og_value = 70;
    err = MyMap_set_pair(&map, key, og_value);
    ASSERT(err == 0);
    value = MyMap_get(&map, key);
    ASSERT(value);
    ASSERT(*value == og_value);
    printfd("Got this value mate %d", *value);
    MyMap_print_bitmap(&map);

    key = INT_MAX;
    og_value = 80;
    err = MyMap_set_pair(&map, key, og_value);
    ASSERT(err == 0);
    value = MyMap_get(&map, key);
    ASSERT(value);
    ASSERT(*value == og_value);
    printfd("Got this value mate %d", *value);
    MyMap_print_bitmap(&map);

    key = 8080;
    og_value = 21;
    err = MyMap_set_pair(&map, key, og_value);
    ASSERT(err == 0);
    value = MyMap_get(&map, key);
    ASSERT(value);
    ASSERT(*value == og_value);
    printfd("Got this value mate %d", *value);
    MyMap_print_bitmap(&map);
    ASSERT(map.pair_amount = 4);


    err = MyMap_pop(&map, key, &out_value);
    ASSERT(err == 0);
    printfd("Popped and got this value %d", *value);
    ASSERT(out_value == og_value);
    ASSERT(map.pair_amount = 3);


    MyMap_print_bitmap(&map);
    ASSERT(map.cols == MAP__DEFAULT_COLS);
    ASSERT(map.rows == MAP__DEFAULT_ROWS);

    MyMap_destroy(&map);
}


int main(void) {
    test1();
    test2();
}
