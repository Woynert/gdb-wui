#include "stdio.h"   // printf
#include "stdbool.h" // true
#include "stdlib.h"  // exit

#define ASSERT(value) \
    do { \
        if ((value) != true) { \
            printf("\nFailed assert %s|%s:%d\n", \
                    __func__, __FILE__, __LINE__); \
            printf(#value); \
            printf("\n"); \
            asm("int3"); \
            exit(1); \
        } \
    } while (0)
