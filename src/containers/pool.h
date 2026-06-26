#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


/* User didn't specify type, using default. */
#ifndef POOL__TYPE
#define POOL__TYPE uint
#endif

/* Token concatenation. */
#define POOL__TOKCAT_(a, b) a ## b
#define POOL__TOKCAT(a, b) POOL__TOKCAT_(a, b)
#ifndef POOL__NAMESPACE
#define POOL__NAMESPACE POOL__TOKCAT(POOL__TYPE, _Pool)
#endif

#if (defined(pfx) | defined(TYPE) | defined(POOL))
#error "These macros should not be defined: pfx, TYPE, POOL"
#endif
#define pfx(name) POOL__TOKCAT(POOL__TOKCAT(POOL__NAMESPACE, _), name)
#define TYPE POOL__TYPE
#define POOL POOL__NAMESPACE


typedef struct pfx(Node) {
    int an_available_id; /* An ID to somewhere inside pool->nodes. */
    bool used;           /* If true, value can be "deleted" (de-ocupied). */
    TYPE value;          /* User data. */
} pfx(Node);

typedef struct POOL {
    int available_count;
    int capacity;
    pfx(Node) *nodes;
} POOL;


/// @returns Error.
static int pfx(create) (POOL *p) {
    *p = (POOL) { 0 };
    p->capacity = 2;
    p->available_count = p->capacity;
    p->nodes = (pfx(Node)*) malloc(sizeof(pfx(Node)) * (size_t)p->capacity);
    if (p->nodes == NULL) { return -1; }

    /* Populate available ids. */
    for (int i = 0; i < p->capacity; ++i) {
        p->nodes[i].an_available_id = i;
        p->nodes[i].used = false;
        p->nodes[i].value = (TYPE) { 0 };
    }
    return 0;
}


static void pfx(destroy) (POOL *p) {
    if (p->nodes != NULL) {
        free(p->nodes);
    }
    *p = (POOL) { 0 };
}



static inline void pfx(_remove_available_id) (POOL *p) {
    p->nodes[0].an_available_id = p->nodes[p->available_count-1].an_available_id;
    --p->available_count;
}
static inline void pfx(_add_available_id) (POOL *p, int id) {
    p->nodes[p->available_count].an_available_id = id;
    ++p->available_count;

    if ((0)) {
        printf("Available ids: [");
        for (int i = 0; i < p->available_count; ++i)
        { printf("%d, ", p->nodes[i].an_available_id); }
        printf("]\n");
    }
}


/// @returns Error.
static int pfx(grow) (POOL *p) {
    int prev_capacity = p->capacity;
    int new_capacity = p->capacity * 2; // <- Grow factor.

    printf("Capacity old %d new %d\n", p->capacity, new_capacity);
    pfx(Node)* new_nodes = (pfx(Node)*) realloc(p->nodes, sizeof(pfx(Node)) * (size_t)new_capacity);

    if (new_nodes == NULL) { return -1; }

    p->capacity = new_capacity;
    p->nodes = new_nodes;

    for (int i = prev_capacity, k = 0; i < p->capacity; ++i, ++k) {
        /* Clear "used" */
        p->nodes[i].used = false;

        /* Populate "available_id" */
        pfx(_add_available_id)(p, i);

        p->nodes[i].value = (TYPE) { 0 };
    }

    return 0;
}


/// @returns ID for newly added element.
/// @reval -1 Error.
static int pfx(insert) (POOL *p, TYPE value) {
    if (p->available_count <= 0) {
        int err = pfx(grow)(p);
        if (err != 0) { return -1; }
    }

    int id = p->nodes[0].an_available_id;
    pfx(_remove_available_id)(p);

    p->nodes[id].value = value;
    p->nodes[id].used = true;

    return id;
}


/// @returns Error.
static int pfx(remove) (POOL *p, int id) {
    if (id < 0 || id >= p->capacity) { return -1; }

    if (!p->nodes[id].used) { return 0; } /* Neutral, no exist so no remove */

    p->nodes[id].used = false;
    pfx(_add_available_id)(p, id);
    return 0;
}


/// @returns A reference to a saved item.
/// @retval NULL. Error.
static TYPE* pfx(get) (POOL *p, int id) {
    if (id < 0 || id >= p->capacity) { return NULL; }
    if (!p->nodes[id].used) { return NULL; }
    return &p->nodes[id].value;
}



#undef pfx
#undef TYPE
#undef POOL
#undef POOL__TYPE
#undef POOL__NAMESPACE
#undef POOL__TOKCAT
#undef POOL__TOKCAT_
