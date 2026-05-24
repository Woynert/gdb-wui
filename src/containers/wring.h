#include <stdlib.h>
#include <stdbool.h>

/*
 * Generic Type Safe Ring
 * Usage:
 *
 * #define WRING__TYPE <type>
 * #define WRING__NAMESPACE <custom name> (optional)
 * #include "wring.h"
 * #undef  WRING__TYPE
 * #undef  WRING__NAMESPACE
 *
 * Example:
 *
 * #define WRING__TYPE int
 * #define WRING__NAMESPACE IntRing // <-- Optional
 * #include "wring.h"
 * IntRing tree = IntRing_create();
 */

/* user didn't specify type, using default */
#ifndef WRING__TYPE
#define WRING__TYPE double
#endif

/* token concatenation */
#define WRING__TOKCAT_(a, b) a ## b
#define WRING__TOKCAT(a, b) WRING__TOKCAT_(a, b)
#ifndef WRING__NAMESPACE
#define WRING__NAMESPACE WRING__TOKCAT(WRING__TYPE, _WRing)
#endif
#define PRE(name) WRING__TOKCAT(WRING__TOKCAT(WRING__NAMESPACE, _), name)
#define TYPE      WRING__TYPE
#define WRing     WRING__NAMESPACE

typedef struct {
    uint  _capacity;
    uint  _tail;
    uint  _head;
    uint  size;
    TYPE *buffer;
} WRing;

static WRing  PRE(create)            (uint capacity);
static void   PRE(free)              (WRing *ring);
static int    PRE(push_head)         (WRing *ring, TYPE item);
static int    PRE(extend_head)       (WRing *ring);
static void   PRE(extend_head_force) (WRing *ring);
static TYPE  *PRE(pop_head)          (WRing *ring);
static TYPE  *PRE(pop_tail)          (WRing *ring);
static TYPE  *PRE(get_from_head)     (WRing *ring, uint pos);
static TYPE  *PRE(get_from_tail)     (WRing *ring, uint pos);
static TYPE  *PRE(get_head)          (WRing *ring);
static TYPE  *PRE(get_tail)          (WRing *ring);
static void   PRE(clear)             (WRing *ring);
static int    PRE(remove_from_tail)  (WRing *ring, uint pos);
#ifdef WRING_ENABLE_COMPARISONS
static bool   PRE(has_item)          (WRing *ring, TYPE item);
#endif
/*
 * TODO:
 * static void PRE(insert_at)
 * static void PRE(sort)
 */

static WRing PRE(create) (uint capacity) {
    WRing ring = { 0 };
    ring._capacity = capacity;
    ring.buffer = (TYPE*)calloc(ring._capacity, sizeof(TYPE));
    return ring;
}

static void PRE(free) (WRing *ring) {
    if (ring->buffer != NULL) {
        free(ring->buffer);
        ring->buffer = NULL;
    }
}

/*
 * @returns Error.
 */
static int PRE(push_head) (WRing *ring, TYPE item) {
	if (ring->size >= ring->_capacity) {
		return -1;
    } else if (ring->size == 0) {
		ring->_head = 0;
		ring->_tail = 0;
    } else {
		++ring->_head;
		if (ring->_head >= ring->_capacity)
            { ring->_head = 0; }
    }
	ring->buffer[ring->_head] = item;
	++ring->size;
	return 0;
}

/*
 * Same as push_head but doesn't insert. The slot will contain old data.
 * @returns Error.
 */
static int PRE(extend_head) (WRing *ring) {
	if (ring->size >= ring->_capacity) {
		return -1;
    } else if (ring->size == 0) {
		ring->_head = 0;
		ring->_tail = 0;
    } else {
		++ring->_head;
		if (ring->_head >= ring->_capacity)
            { ring->_head = 0; }
    }
	++ring->size;
	return 0;
}

/*
 * If already full will pop tail to make space, otherwise same as extend_head.
 */
static void PRE(extend_head_force) (WRing *ring) {
	while (ring->size >= ring->_capacity) {
		PRE(pop_tail)(ring);
    }
    PRE(extend_head)(ring);
}

/*
 * @returns Reference to popped item.
 */
static TYPE *PRE(pop_tail) (WRing *ring) {
	if (ring->size <= 0) { return NULL; }
	
	TYPE *item = &ring->buffer[ring->_tail];
	
	if (ring->size > 1) {
		++ring->_tail;
		if (ring->_tail >= ring->_capacity) {
            ring->_tail = 0;
        }
    }
	
	--ring->size;
	return item;
}

/*
 * @returns Reference to popped item.
 */
static TYPE *PRE(pop_head) (WRing *ring) {
	if (ring->size <= 0) { return NULL; }
	
	TYPE *item = &ring->buffer[ring->_head];
	
	if (ring->size > 1) {
        if (ring->_head == 0) {
            ring->_head = ring->_capacity -1;
        } else {
            --ring->_head;
        }
    }
	
	--ring->size;
	return item;
}

/*
 * @returns Reference to item.
 */
static TYPE *PRE(get_from_head) (WRing *ring, uint pos) {
    if (pos >= ring->size) { return NULL; }
	uint index = (ring->_head - pos) % ring->_capacity;
	return &ring->buffer[index];
}

/* @returns Reference to item. */
static TYPE *PRE(get_from_tail) (WRing *ring, uint pos) {
    if (pos >= ring->size) { return NULL; }
	uint index = (ring->_tail + pos) % ring->_capacity;
	return &ring->buffer[index];
}

static TYPE *PRE(get_head) (WRing *ring) {
    if (ring->size <= 0) { return NULL; }
    return &ring->buffer[ring->_head];
}

static TYPE *PRE(get_tail) (WRing *ring) {
    if (ring->size <= 0) { return NULL; }
    return &ring->buffer[ring->_tail];
}

static void PRE(clear) (WRing *ring) {
    ring->_tail = 0;
    ring->_head = 0;
    ring->size = 0;
    // TODO: Implement WRing_memset_zero and WRing_set_all_to_value
    // memset(ring->buffer, 0, ring->_capacity); DANGEROUS
}

/*
 * @note Will overwrite values. If your TYPE owns memory it will leak.
 * @returns Error.
 */
static int PRE(remove_from_tail) (WRing *ring, uint pos) {
	if (ring->size <= 0) { return -1; }
	if (pos >= ring->size) { return -2; }

    uint index_curr, index_next;
    for (uint i = pos; i < ring->size; ++i) {
        index_curr = (ring->_tail + i) % ring->_capacity;
        if (i < (ring->size -1)) {
            index_next = (ring->_tail + i + 1) % ring->_capacity;
            ring->buffer[index_curr] = ring->buffer[index_next];
        }
        // garbage value remains on moved object previous position
    }

    ring->_head = (ring->_head -1) % ring->_capacity;
	--ring->size;
	return 0;
}

#ifdef WRING_ENABLE_COMPARISONS

static bool PRE(has_item) (WRing *ring, TYPE item) {
	uint tail = ring->_tail, head = ring->_head;

	if (ring->size == 0) return false;
	if (tail < head) {
		for (uint i = tail; i <= head; ++i) {
			if (ring->buffer[i] == item) return true;
		}
	}
	if (tail > head) {
		for (uint i = tail; i < ring->_capacity; ++i) {
			if (ring->buffer[i] == item) return true;
		}
		for (uint i = 0; i <= head; ++i) {
			if (ring->buffer[i] == item) return true;
		}
	}
	return false;
}

#endif // WRING_ENABLE_COMPARISONS

#undef PRE
#undef TYPE
//#undef WRING_TYPE
//#undef WRING_PREFIX
//#undef WRING_ENABLE_COMPARISONS
