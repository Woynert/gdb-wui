/*
   Data structure properties:
   * Unlimited growth.
   * Can add new string buffers.
   * Cannot delete existing buffers.
   * However, can clear the whole thing.
   */

#ifndef STRINGPOOL
#define STRINGPOOL

#include "strbuf.h"
#include "strbuf_extra.h"

typedef struct Stringpool__Pair {
    int start;
    int size;
} Stringpool__Pair;

#define DYNA__TYPE Stringpool__Pair
#include "da.h"

typedef struct Stringpool {
    Stringpool__Pair_Dyna pairs; // Maps PAIR_ID to -> Pair.
    strbuf_t *buffer;
} Stringpool;

static int Stringpool_append(Stringpool *s, strview_t string);

static void Stringpool_create(Stringpool *s) {
    *s = (Stringpool) { 0 };
    s->pairs = Stringpool__Pair_Dyna_create();
    s->buffer = strbuf_create(0, NULL);

    /* @note: This will make zero IDS always point to an empty view. */
    Stringpool_append(s, cstr_SL(""));
}

static void Stringpool_destroy(Stringpool *s) {
    Stringpool__Pair_Dyna_free(&s->pairs);
    strbuf_destroy(&s->buffer);
    *s = (Stringpool) { 0 };
}

/// @returns ID of newly saved string.
/// @reval   -1 Error.
static int Stringpool_append(Stringpool *s, strview_t string) {
    Stringpool__Pair pair = {
        .start = s->buffer->size,
        .size = string.size
    };
    strbuf_append_strview(&s->buffer, string);
    return Stringpool__Pair_Dyna_append(&s->pairs, pair);
}

/// @returns View inside buffer.
/// @retval STRVIEW_INVALID on Error.
static strview_t Stringpool_get(Stringpool *s, int pair_id) {
    Stringpool__Pair *pair = Stringpool__Pair_Dyna_get_safe(&s->pairs, pair_id);
    if (pair == NULL) { return STRVIEW_INVALID; }
    return (strview_t) {
        .data = s->buffer->cstr + pair->start,
        .size = pair->size
    };
}

static void Stringpool_clear_preserving(Stringpool *s) {
    Stringpool__Pair_Dyna_clear_preserving(&s->pairs);
    strbuf_empty(&s->buffer);
}

static void Stringpool_clear_freeing(Stringpool *s) {
    Stringpool__Pair_Dyna_clear_freeing(&s->pairs);
    strbuf_empty(&s->buffer);
    strbuf_shrink(&s->buffer);
}

#endif // !STRINGPOOL
