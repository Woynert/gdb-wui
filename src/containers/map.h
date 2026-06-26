/*
   Features:
   * Unordered map, order may change between operations.

   @notes:
   * Growing vertically: Adds new rows, No rehash, Cheap.
   * Growing horizontally: Adds new cols or "buckets", Needs rehash, Expensive.

   Optional macro: MAP__DEBUG, MAP__ENABLE_ASSERT

 */

#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


/* Key type not specified, using default. */
#ifndef MAP__KEY
#define MAP__KEY int
#endif

/* Type not specified, using default. */
#ifndef MAP__TYPE
#define MAP__TYPE uint
#endif

/* Token concatenation. */
#define MAP__TOKCAT_(a, b) a ## b
#define MAP__TOKCAT(a, b) MAP__TOKCAT_(a, b)
#ifndef MAP__NAMESPACE
#define MAP__NAMESPACE MAP__TOKCAT(MAP__TYPE, _Map)
#endif
#define MAP__PREFIX(name) MAP__TOKCAT(MAP__TOKCAT(MAP__NAMESPACE, _), name)


#define POOL__TYPE MAP__TYPE
#define POOL__NAMESPACE MAP__PREFIX(Pool)
#include "pool.h"


typedef struct MAP__PREFIX(Pair) {
    int value_id; /* -1 Means available. */
    MAP__KEY key;
} MAP__PREFIX(Pair);


#define SIZEDBUF__TYPE MAP__PREFIX(Pair)
#define SIZEDBUF__NAMESPACE MAP__PREFIX(PairBuf)
#include "sizedbuffer.h"


#if (defined(pfx) | defined(TYPE) | defined(KEY) | defined(Map))
#error "These macros should not be defined: pfx, TYPE, KEY, Map"
#endif
#define pfx MAP__PREFIX
#define TYPE MAP__TYPE
#define KEY MAP__KEY
#define Map MAP__NAMESPACE


#define MAP__DEFAULT_COLS 8
#define MAP__DEFAULT_ROWS 2


typedef struct Map {
    int cols;
    int rows;
    pfx(Pool) pool;
    pfx(PairBuf) pairs;
    int pair_amount; /* Used only to calculate the factor. */
} Map;


static int     pfx(create) (Map *m);
static void    pfx(destroy) (Map *m);
static int     pfx(grow_vertically) (Map *m);
static int     pfx(grow_and_rehash) (Map *m);
static int     pfx(set_pair) (Map *m, const KEY key, const TYPE value);
static TYPE   *pfx(get) (Map *m, const KEY key);
static int     pfx(pop) (Map *m, const KEY key, TYPE *out_value_optional);
static void    pfx(print_bitmap) (Map *m);
static int     pfx(_hash) (const Map *m, const KEY key);
static int     pfx(_grow) (Map *m, int new_rows, int new_cols);
static void    pfx(_init_pairs) (Map *m, int from, int end);
static int     pfx(_set_pair) (Map *m, const KEY key, const int value_id, const bool can_rehash);
static size_t  pfx(_size_t_min) (size_t a, size_t b);
static inline void pfx(_pair_populate) (pfx(Pair) *pair, const KEY key, const int value_id);


#ifdef MAP__DEBUG
#define MAP__PRINT(...) \
    do { printf("\033[33m"); printf(__VA_ARGS__); \
    printf("\033[0m" " %s:%s:%d\n", __func__, __FILE__, __LINE__); } while (0)
#else
#define MAP__PRINT(...) do { } while(0)
#endif


#ifdef MAP__ENABLE_ASSERT
#define MAP__ASSERT(value) \
    do { if (!!(value) != true) { \
    printf("\nFAILED ASSERT %s|%s:%d\n", __func__, __FILE__, __LINE__); \
    printf(#value); printf("\n"); __asm__("int3"); exit(1); } } while (0)
#else
#define MAP__ASSERT(x)
#endif


static TYPE   *pfx(get) (Map *m, const KEY key) {
    int bucket_col = pfx(_hash)(m, key);
    pfx(Pair) *pair = NULL;

    for (int i = bucket_col; i < m->pairs.size; i += m->cols) { /* Iterate rows in column. */
        pair = &m->pairs.items[i];
        if (pair->key == key) {
            return pfx(Pool_get)(&m->pool, pair->value_id);
        }
        if (pair->value_id == -1) { /* Stop looking. */
            break;
        }
    }
    return NULL;
}


static int pfx(pop) (Map *m, const KEY key, TYPE *out_value_optional) {
    int bucket_col = pfx(_hash)(m, key);

    pfx(Pair) *pair_target = NULL;
    pfx(Pair) *pair_last = NULL;

    {
        pfx(Pair) *pair = NULL;
        pfx(Pair) *pair_prev = NULL;

        for (int i = bucket_col; i < m->pairs.size; i += m->cols) { /* Iterate rows in column. */
            pair_prev = pair;
            pair = &m->pairs.items[i];
            if (pair->key == key) {
                pair_target = pair;
            }
            if (pair->value_id == -1) { /* Stop looking. */
                break;
            }
        }
        pair_last = pair_prev;
    }

    if (!pair_target) { return -1; }

    if (out_value_optional != NULL) {
        TYPE *value = pfx(Pool_get)(&m->pool, pair_target->value_id);
        if (value != NULL) {
            *out_value_optional = *value;
        }
    }

    // Remove and invalidate

    int err = pfx(Pool_remove)(&m->pool, pair_target->value_id);
    MAP__ASSERT(err == 0);
    if (err != 0) {
        MAP__PRINT("WAR: Couldn't remove from pool.");
    }

    pair_target->value_id = -1;

    --m->pair_amount;

    // No pair last means rows == 1.
    if (!pair_last) { return 0; }

    // Swap
    pfx(Pair) pair_temp = *pair_target;
    *pair_target = *pair_last;
    *pair_last = pair_temp;
    return 0;
}


int pfx(_set_pair) (Map *m, const KEY key, int value_id, const bool can_rehash);


void pfx(_init_pairs) (Map *m, int from, int end) {
    for (int i = from; i < end; ++i) {
        m->pairs.items[i].value_id = -1;
    }
}


int pfx(_grow) (Map *m, int new_rows, int new_cols) {

    int old_size = m->pairs.size;
    int new_size = new_rows * new_cols;

    int err = pfx(PairBuf_grow_zero)(&m->pairs, new_size);
    if (err != 0) {
        MAP__PRINT("ERR: Can't grow, out of memory?");
        return -1;
    }

    pfx(_init_pairs)(m, old_size, m->pairs.size);

    m->rows = new_rows;
    m->cols = new_cols;
    return 0;
}


int pfx(create) (Map *m) {
    *m = (Map) { 0 };

    m->cols = MAP__DEFAULT_COLS;
    m->rows = MAP__DEFAULT_ROWS;

    int err = pfx(Pool_create)(&m->pool);
    if (err != 0) {
        return -1;
    }

    m->pairs = pfx(PairBuf_create)();

    err = pfx(_grow)(m, m->rows, m->cols);
    if (err != 0) {
        pfx(Pool_destroy)(&m->pool);
        return -1;
    }

    return 0;
}


void pfx(destroy) (Map *m) {
    pfx(Pool_destroy)(&m->pool);
    pfx(PairBuf_destroy)(&m->pairs);
    *m = (Map) { 0 };
}


size_t pfx(_size_t_min) (size_t a, size_t b) { return a > b ? b : a; }


int pfx(_hash) (const Map *m, const KEY key) {
    int key_fragment = 0;
    memcpy(&key_fragment, &key, pfx(_size_t_min)(sizeof(int), sizeof(KEY)));
    #define MAP__PYTHON_MODULO(n, M) ((((n) % (M)) + (M)) % (M))
    MAP__PRINT("(key %d) %d %% %d = %d", key, key_fragment, m->cols, MAP__PYTHON_MODULO(key_fragment, m->cols));
    return MAP__PYTHON_MODULO(key_fragment, m->cols);
    #undef MAP__PYTHON_MODULO
}



int pfx(grow_vertically) (Map *m) {
    MAP__PRINT("GROW VERTICALLY");
    pfx(_grow)(m, m->rows * 2, m->cols);
    return 0;
}



int pfx(grow_and_rehash) (Map *m) {
    Map new_map = *m;

    // Create a new Buffer.

    new_map.pair_amount = 0;
    new_map.pairs = pfx(PairBuf_create)();
    int err = pfx(_grow)(&new_map, MAP__DEFAULT_ROWS, m->cols * 2);

    if (err != 0) {
        pfx(PairBuf_destroy)(&new_map.pairs);
        MAP__PRINT("ERR: out of memory?");
        return -1;
    }

    // Insert pairs from the old to the new.

    MAP__PRINT("--- START REHASH INSERT");
    for (int i = 0; i < m->pairs.size; ++i) {
        pfx(Pair) *pair = &m->pairs.items[i];
        if (pair->value_id == -1) { continue; }

        err = pfx(_set_pair) (&new_map, pair->key, pair->value_id, false);
    }
    MAP__PRINT("--- END REHASH INSERT");

    pfx(PairBuf_destroy)(&m->pairs);
    m->cols = new_map.cols;
    m->rows = new_map.rows;
    m->pairs = new_map.pairs;

    return 0;
}




/// @returns Error.
int pfx(set_pair) (Map *m, const KEY key, const TYPE value) {

    /* @note: In the case the key exists and we overwrite it, this insertion
       is unnecessary. But, there's no downside if the pool has available
       space, which is most lickely.
       */
    int value_id = pfx(Pool_insert)(&m->pool, value);
    if (value_id == -1) { MAP__PRINT("ERR: Out of memory?"); return -1; }

    int err = pfx(_set_pair) (m, key, value_id, true);
    if (err != 0) { return -1; }


    return 0;
}


static inline void pfx(_pair_populate) (pfx(Pair) *pair, const KEY key, const int value_id)
{ *pair = (pfx(Pair)) { .key = key, .value_id = value_id }; }


/*
   set_pair(...)
   ↓
   Two ops in one. (1) Check if key already exist and (2) Find available row.
   ↓
   Key already exists? -> Yes -> Overwrite value, exit.
   ↓
   No
   ↓
   Check: if we insert this, should we rehash?
   ↓                     ↓
   Yes                   No
   ↓                     ↓
   Grow and Rehash.      ↓
   ↓                     ↓
   ↓                     Found row from previus step? -> Yes -> Insert and exit.
   ↓                                                 ↓
   Find row. Found row? -> Yes -> Insert and exit.   ↓
   ↓                                                 No
   No                                                ↓
   ↓←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
   ↓
   Grow vertically. Failed? -> Yes, Exit with error.
   ↓
   No
   ↓
   Look for row to insert pair. Found row? -> No -> Exit with error.
   ↓
   Yes -> Insert and exit.
*/
/// @param can_rehash. Used internally when reinserting from a rehash. To
///                    prevent rehashing (shouldn't happen but still).
/// @returns Error.
int pfx(_set_pair) (Map *m, const KEY key, const int value_id, const bool can_rehash) {
    MAP__PRINT("--- SET PAIR CALL");

    bool found = false;
    bool key_exists = false;
    pfx(Pair) *pair = NULL;
    int bucket_col = pfx(_hash)(m, key);
    MAP__PRINT("current bucket_col %d", bucket_col);

    for (int i = bucket_col; i < m->pairs.size; i += m->cols) { // Iterate rows.
        pair = &m->pairs.items[i];
        //MAP__PRINT("Checking pair i %d (key %d, value_id %d)", i, pair->key, pair->value_id);
        if (pair->key == key) {
            key_exists = true;
            break;
        }
        if (pair->value_id == -1) {
            found = true;
            break;
        }
    }

    if (key_exists) {
        // Remove existing value. (should be cheap).
        int err = pfx(Pool_remove)(&m->pool, pair->value_id);
        if (err != 0) { MAP__PRINT("WAR: Couldn't remove value."); }

        pair->value_id = value_id;
        return 0;
    }

    // Should rehash if inserted?

    float factor = (float)(m->pair_amount +1) / (float)m->cols;
    bool should_rehash = factor >= 0.75 && can_rehash;

    if (!should_rehash && found) // Found available pair from before.
    {
        pfx(_pair_populate)(pair, key, value_id);
        return 0;
    }

    if (should_rehash) {
        MAP__PRINT("Rehashing with factor %f", factor);
        int err = pfx(grow_and_rehash)(m);
        if (err != 0) { MAP__PRINT("WAR: Couldn't grow_and_rehash, continuing..."); }

        // Try find available pair.

        bucket_col = pfx(_hash)(m, key);
        found = false;

        MAP__PRINT("current bucket_col %d", bucket_col);

        for (int i = bucket_col; i < m->pairs.size; i += m->cols) {
            pair = &m->pairs.items[i];
            MAP__PRINT("Checking pair i %d (key %d, value_id %d)", i, pair->key, pair->value_id);
            if (pair->value_id == -1) { found = true; break; }
        }
        if (found) {
            pfx(_pair_populate)(pair, key, value_id);
            return 0;
        }
    }

    // Did not found available row, let's grow vertically.
    int err = pfx(grow_vertically)(m);
    if (err == -1) { MAP__PRINT("ERR: Out of memory?"); return -1; }

    // Try find available pair again.
    found = false;
    for (int i = bucket_col; i < m->pairs.size; i += m->cols) {
        pair = &m->pairs.items[i];
        if (pair->value_id == -1) { found = true; break; }
    }
    if (found) {
        pfx(_pair_populate)(pair, key, value_id);
        return 0;
    }

    return -1;
}


void pfx(print_bitmap) (Map *m) {
    printf("MAP size %d, cols %d, rows %d\n", m->pairs.size, m->cols, m->rows);

    pfx(Pair) *pair = NULL;
    TYPE *value = NULL;

    for (int row = 0; row < m->rows; ++row) {
        printf("[");
        for (int col = 0; col < m->cols; ++col) {

            pair = &m->pairs.items[col + row * m->cols];
            value = NULL;

            if (pair->value_id == -1) {
                printf("xx,xx|");
            } else {
                value = pfx(Pool_get)(&m->pool, pair->value_id);
                printf("%d,%d|", pair->key, *value);
            }

        }
        printf("]\n");
    }
    printf("\n");
}



#undef MAP__KEY
#undef MAP__TYPE
#undef MAP__TOKCAT_
#undef MAP__TOKCAT
#undef MAP__NAMESPACE
#undef MAP__PREFIX
#undef pfx
#undef TYPE
#undef KEY
#undef Map
#undef MAP__PRINT
#undef MAP__ASSERT
