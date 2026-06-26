#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/*
 * Type safe resizable buffer.
 *
 * Usage:
 *
 * #define SIZEDBUF__TYPE <type>
 * #define SIZEDBUF__NAMESPACE <custom name> (optional)
 * #define SIZEDBUF__ENABLE_COMPARISONS      (optional)
 * #define SIZEDBUF__IS_BUFFER               (optional)
 * #include "sizedbuffer.h"
*/

/* User didn't specify type, using default. */
#ifndef SIZEDBUF__TYPE
#define SIZEDBUF__TYPE uint
#define SIZEDBUF__ENABLE_COMPARISONS
#endif

/* Token concatenation. */
#define SIZEDBUF__TOKCAT_(a, b) a ## b
#define SIZEDBUF__TOKCAT(a, b) SIZEDBUF__TOKCAT_(a, b)
#ifndef SIZEDBUF__NAMESPACE
#define SIZEDBUF__NAMESPACE SIZEDBUF__TOKCAT(SIZEDBUF__TYPE, _Sizedbuffer)
#endif

#if (defined(pfx) | defined(TYPE) | defined(Sizedbuffer))
#error "These macros should not be defined: pfx, TYPE, Sizedbuffer"
#endif
#define pfx(name) SIZEDBUF__TOKCAT(SIZEDBUF__TOKCAT(SIZEDBUF__NAMESPACE, _), name)
#define TYPE SIZEDBUF__TYPE
#define Sizedbuffer SIZEDBUF__NAMESPACE


typedef struct {
    int size;
    TYPE *items;
} Sizedbuffer;


static Sizedbuffer pfx(create)          (void);
static void        pfx(destroy)         (Sizedbuffer *b);
static int         pfx(resize)          (Sizedbuffer *b, int new_capacity);
static int         pfx(grow_zero)       (Sizedbuffer *b, int min_capacity);
static int         pfx(grow_trash)      (Sizedbuffer *b, int min_capacity);
static void        pfx(fill_zero)       (Sizedbuffer *b);
static void        pfx(fill_with_value) (Sizedbuffer *b, TYPE item);
static int         pfx(set_safe)        (Sizedbuffer *b, int index, TYPE item);
static TYPE *      pfx(get_safe)        (const Sizedbuffer *b, int index);



static inline Sizedbuffer pfx(create) (void) {
    return (Sizedbuffer) { 0 };
}


static inline void pfx(destroy) (Sizedbuffer *b) {
    if (b->items != NULL) {
        free(b->items);
    }
    *b = (Sizedbuffer) { 0 };
}


/// @returns error
static int pfx(resize) (Sizedbuffer *b, int new_size) {
    if (new_size < 0) { return -1; }
    if (b->size == new_size) { return 0; }

    TYPE *new_ptr = NULL;

    if (b->items != NULL) {
        new_ptr = (TYPE *)realloc(b->items, sizeof(TYPE) * (size_t)new_size);
    } else {
        *b = (Sizedbuffer) { 0 };
        new_ptr = (TYPE *)malloc(sizeof(TYPE) * (size_t)new_size);
    }

    if (new_ptr == NULL) { return -1; }
    b->size = new_size;
    b->items = new_ptr;
    return 0;
}


/* @returns Error */
static inline int pfx(grow_zero) (Sizedbuffer *b, int minimum_size) {
    if (b->size >= minimum_size) { return 0; }
    
    int old_size = b->size;

    int err = pfx(resize)(b, minimum_size);
    if (err != 0) { return -1; }

    memset(b->items + old_size, 0, sizeof(TYPE) * (size_t)(b->size - old_size));

    return err;
}


/* @returns Error */
static inline int pfx(grow_trash) (Sizedbuffer *b, int minimum_size) {
    if (b->size >= minimum_size) { return 0; }
    return pfx(resize)(b, minimum_size);
}


static void pfx(fill_zero) (Sizedbuffer *b) {
    memset(b->items, 0, sizeof(TYPE) * (size_t)b->size);
}


static void pfx(fill_with_value) (Sizedbuffer *b, TYPE item) {
    for (int i = 0; i < b->size; ++i) {
        b->items[i] = item;
    }
}


/* @note You can directly access items instead too. */
/* @returns Error. */
static int pfx(set_safe) (Sizedbuffer *b, int index, TYPE item) {
    if (index < 0 || index >= b->size) { return -1; }
    b->items[index] = item;
    return 0;
}


/* @note Safe get which checks boundares */
/* @note You can directly access items instead too. */
/* @returns Pointer or NULL. */
static TYPE *pfx(get_safe) (const Sizedbuffer *b, int index) {
    if (index < 0 || index >= b->size) { return NULL; }
    return &b->items[index];
}


#undef pfx
#undef TYPE
#undef Sizedbuffer
#undef SIZEDBUF__TYPE
#undef SIZEDBUF__NAMESPACE
#undef SIZEDBUF__ENABLE_COMPARISONS
#undef SIZEDBUF__TOKCAT_
#undef SIZEDBUF__TOKCAT
