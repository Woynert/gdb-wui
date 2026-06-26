#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/*
 * Type safe dynamic array.
 *
 * Note: This container uses signed int for size and capacity variables. The
 *     maximum size allowed is INT_MAX from "limits.h". Usually that's
 *     2_147_483_647 items (0x7fffffff). For more info read:
 *     https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1428r0.pdf
 *
 * Usage:
 *
 * #define DYNA__TYPE <type>
 * #define DYNA__NAMESPACE <custom name> (optional)
 * #define DYNA__ENABLE_COMPARISONS      (optional)
 * #include "dyna.h"
*/

/* User didn't specify type, using default. */
#ifndef DYNA__TYPE
#define DYNA__TYPE uint
#define DYNA__ENABLE_COMPARISONS
#endif

/* Token concatenation. */
#define DYNA__TOKCAT_(a, b) a ## b
#define DYNA__TOKCAT(a, b) DYNA__TOKCAT_(a, b)
#ifndef DYNA__NAMESPACE
#define DYNA__NAMESPACE DYNA__TOKCAT(DYNA__TYPE, _Dyna)
#endif

#if (defined(pfx) | defined(TYPE) | defined(Dyna))
#error "These macros should not be defined: pfx, TYPE, Dyna"
#endif
#define pfx(name) DYNA__TOKCAT(DYNA__TOKCAT(DYNA__NAMESPACE, _), name)
#define TYPE DYNA__TYPE
#define Dyna DYNA__NAMESPACE

typedef struct {
    int capacity;
    int size;
    TYPE *items;
} Dyna;

typedef struct {
    int __next_index;
    int index;
    TYPE *item;
} pfx(Iterator);

static Dyna  pfx(create)                      (void);
static void  pfx(free)                        (Dyna *da);
static int   pfx(resize)                      (Dyna *da, int new_capacity);
static void  pfx(clear_freeing)               (Dyna *da);
static void  pfx(clear_preserving)            (Dyna *da);
static int   pfx(append)                      (Dyna *da, TYPE item);
static int   pfx(grow)                        (Dyna *da, int min_capacity);
static void  pfx(fill_zero)                   (Dyna *da);
static void  pfx(fill_trash)                  (Dyna *da);
static void  pfx(fill_with_value)             (Dyna *da, TYPE item);
static int   pfx(shrink)                      (Dyna *da);
static int   pfx(insert_at_preserve_order)    (Dyna *da, int index, TYPE item);
static int   pfx(remove_at)                   (Dyna *da, int index);
static int   pfx(pop_at_preserve_order)       (Dyna *da, int index);
static int   pfx(set)                         (Dyna *da, int index, TYPE item);
static TYPE *pfx(get_safe)                    (const Dyna *da, int index);
static bool  pfx(get_next)                    (const Dyna *da, pfx(Iterator) *it);
#ifdef DYNA__ENABLE_COMPARISONS
static bool  pfx(has)                         (const Dyna *da, TYPE value);
static bool  pfx(find)                        (const Dyna *da, TYPE value, int *out_index);
static int   pfx(remove_first_instance)       (Dyna *da, TYPE value);
static void  pfx(sort)                        (Dyna *r);
static void  pfx(sort_range)                  (TYPE *buffer, int from, int to);
typedef struct { int a; int b; } pfx(_Pair);
static pfx(_Pair) pfx(_partition)             (TYPE *buffer, int from, int to);
#endif // !DYNA__ENABLE_COMPARISONS
static bool  pfx(_add_will_overflow_int)      (int a, int b);
static int   pfx(_round_up_capacity)          (int capacity);


static inline Dyna pfx(create) (void) {
    return (Dyna) { 0 };
}


static inline void pfx(free) (Dyna *da) {
    if (da->items != NULL) {
        free(da->items);
    }
    *da = (Dyna) { 0 };
}


/// @returns error
static int pfx(resize) (Dyna *da, int new_capacity) {
    if (new_capacity < 0) { return -1; }
    if (da->capacity == new_capacity) { return 0; }

    TYPE *new_ptr = NULL;

    if (da->items == NULL) {
        *da = (Dyna) { 0 };
        new_ptr = (TYPE *)malloc(sizeof(TYPE) * (size_t)new_capacity);
        if (new_ptr == NULL) { return -1; }
    } else {
        new_ptr = (TYPE *)realloc(da->items, sizeof(TYPE) * (size_t)new_capacity);
        if (new_ptr == NULL) { return -1; }
    }

    da->capacity = new_capacity;
    da->items = new_ptr;
    return 0;
}


/// Appends item at the end.
/// @returns index or -1.
/// @reval   -1 Error.
static inline int pfx(append) (Dyna *da, TYPE item) {
    if (da->items == NULL) {
        const int INITIAL_CAPACITY = 2;
        if (pfx(resize)(da, INITIAL_CAPACITY) != 0) { return -1; }
    }
    else if (da->size >= da->capacity) {
        int new_capacity =
            pfx(_add_will_overflow_int(da->capacity, da->capacity)) ?
                INT_MAX : da->capacity * 2;

        if (pfx(resize)(da, new_capacity) != 0) { return -1; }

        /* @note: Recommended grow ratio from 1.5 to 2.0. */
        /* @note: What about a setting to cap grow at steps of say 500MB? */
    }
    if (da->size >= da->capacity) {
        return -1;
        /* @note: Should only happen when already at full capacity INT_MAX.*/
    }

    da->items[da->size] = item;
    ++da->size;
    return da->size -1;
}


/* @returns Error */
static inline int pfx(grow) (Dyna *da, int minimum_capacity) {
    if (da->capacity >= minimum_capacity) { return 0; }
    return pfx(resize)(da, minimum_capacity);
    /*
     * @note Maybe it could be useful to have an option to round_up_capacity
     *     maybe in powers of 2 or something... Maybe even a configurable grow
     *     ratio.
     * int new_capacity = pfx(_round_up_capacity)(minimum_capacity);
     */
}


static int pfx(shrink) (Dyna *da) {
    return pfx(resize)(da, da->size);
}

static void pfx(fill_trash) (Dyna *da) {
    da->size = da->capacity;
}

static void pfx(fill_zero) (Dyna *da) {
    da->size = da->capacity;
    memset(da->items, 0, sizeof(TYPE) * (size_t)da->capacity);
}

static void pfx(fill_with_value) (Dyna *da, TYPE item) {
    da->size = da->capacity;
    for (int i = 0; i < da->capacity; ++i) {
        da->items[i] = item;
    }
}


/* @note You can directly access items instead too. */
/* @returns Error. */
static int pfx(set) (Dyna *da, int index, TYPE item) {
    if (index < 0 || index >= da->size) { return -1; }
    da->items[index] = item;
    return 0;
}


/* @note Safe get which checks boundares */
/* @note You can directly access items instead too. */
/* @returns Pointer or NULL. */
static TYPE *pfx(get_safe) (const Dyna *da, int index) {
    if (index < 0 || index >= da->size) { return NULL; }
    return &da->items[index];
}


/// Will alter the order.
/// Removing while iterating is Undefined Behaviour.
/// @returns error
static int pfx(remove_at) (Dyna *da, int index) {
    if (index < 0 || index >= da->size) { return -1; }
    da->items[index] = da->items[da->size -1];
    --da->size;
    return 0;
}


/// Clears the array freeing old memory.
static void pfx(clear_freeing) (Dyna *da) {
    pfx(free)(da);
}


/// Clears the array but preserves the allocated memory.
static void pfx(clear_preserving) (Dyna *da) {
    da->size = 0;
}


/// @returns error
static int pfx(insert_at_preserve_order) (Dyna *da, int index, TYPE item) {
    if (index < 0 || index > da->size) {
        return -1;
    }
    pfx(append)(da, (TYPE){0}); // Ensure space.
    memmove(da->items +index +1, da->items +index, sizeof(TYPE) * (size_t)(da->size -1 -index));
    da->items[index] = item;
    return 0;
}


/// @returns error
static int pfx(pop_at_preserve_order) (Dyna *da, int index) {
    if (index < 0 || index > da->size) {
        return -1;
    }
    memmove(da->items +index, da->items +index +1, sizeof(TYPE) * (size_t)(da->size -index));
    --da->size;
    return 0;
}


/// @note To create and Iterator just zero initialize one: Iterator it = { 0 };
/// @note Modifying the dynamic array while iterating is UB.
/// @param[out] it iterator
/// @retval true  Can continue iterating.
/// @retval false End reached
static bool pfx(get_next) (const Dyna *da, pfx(Iterator) *it) {
    if (it->__next_index < 0 || it->__next_index >= da->size) {
        it->item = NULL;
        return false;
    }

    it->item = &da->items[it->__next_index];
    it->index = it->__next_index;
    ++it->__next_index;
    return true;
}


#ifdef DYNA__ENABLE_COMPARISONS

static pfx(_Pair) pfx(_partition) (TYPE *buffer, int from, int to) {

    int left = from;
    int eq = from;
    int right = to;
    TYPE pivot = buffer[(from + to) / 2];

    while (eq <= right) {

        if (buffer[eq] < pivot) {
            TYPE swap = buffer[eq];
            buffer[eq] = buffer[left];
            buffer[left] = swap;

            ++left;
            ++eq;
        } else if (buffer[eq] > pivot) {
            TYPE swap = buffer[eq];
            buffer[eq] = buffer[right];
            buffer[right] = swap;

            --right;
        } else {
            ++eq;
        }
    }

    return (pfx(_Pair)){left, right};
}

static void pfx(sort_range) (TYPE *buffer, int from, int to) {
    if (from >= 0 && from < to) {
        pfx(_Pair) partition_index = pfx(_partition)(buffer, from, to);
        if (from +1 < partition_index.a) {
            pfx(sort_range) (buffer, from, partition_index.a -1);
        }
        if (partition_index.b +1 < to) {
            pfx(sort_range) (buffer, partition_index.b +1, to);
        }
    }
}

static void pfx(sort) (Dyna *r) {
    if (r->size == 0) return;
    pfx(sort_range) (r->items, 0, r->size-1);
}

static bool pfx(has) (const Dyna *da, TYPE value) {
    for (int i = 0; i < da->size; ++i) {
        TYPE *stored_value = &da->items[i];
        if (*stored_value == value) {
            return true;
        }
    }
    return false;
}

/// @param[out] out_index Item index if found
/// @returns true if found
static bool pfx(find) (const Dyna *da, TYPE value, int *out_index) {
    for (int i = 0; i < da->size; ++i) {
        TYPE *stored_value = &da->items[i];
        if (*stored_value == value) {
            *out_index = i;
            return true;
        }
    }
    return false;
}

/// @returns error
static int pfx(remove_first_instance) (Dyna *da, TYPE value) {
    int item_index = 0;
    bool found = pfx(find)(da, value, &item_index);
    if (!found) {
        return -1;
    }
    pfx(remove_at)(da, item_index);
    return 0;
}

#endif // !DYNA__ENABLE_COMPARISONS


/* @note Extracted from the excellent mickjc750/str. */
static bool pfx(_add_will_overflow_int) (int a, int b) {
    int c = a;
    c += b;
    return ((a < 0) == (b < 0) && (a < 0) != (c < 0));
}


/*
 * @note Extracted from the excellent mickjc750/str.
 * @note The buffer size can only ever be shrunk by calling shrink(), which
 *       shrinks it to the minimum needed. Higher values will reduce calls to the
 *       allocator, at the expense of more memory overhead.
 * @note The buffer capacity is rounded up to a multiple of this when
 *       calling grow().
 */
static int pfx(_round_up_capacity) (int capacity) {
    const int CAPACITY_GROW_STEP = 16;
    int remainder = capacity % CAPACITY_GROW_STEP;
    if (remainder) {
        if (!pfx(_add_will_overflow_int)(capacity, CAPACITY_GROW_STEP -remainder)) {
            capacity += CAPACITY_GROW_STEP -remainder;
        } else {
            capacity = INT_MAX;
        }
    }
    return capacity;
}


#undef DYNA__TOKCAT_
#undef DYNA__TOKCAT
#undef pfx
#undef TYPE
#undef Dyna
#undef DYNA__TYPE
#undef DYNA__NAMESPACE
#undef DYNA__ENABLE_COMPARISONS
