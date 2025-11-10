#ifndef STRBUF_EXTRA_H
#define STRBUF_EXTRA_H

#include "strbuf.h"

static inline int _strbuf_int_min(int a, int b) { return a < b ? a : b; }
static strview_t strview_of_buf(strbuf_t* buf)
{
    strview_t str = STRVIEW_INVALID;
    if(buf)
    {
        str.data = buf->cstr;
        str.size = buf->size;
    };
    return str;
}

#define STRBUF_STATIC_INIT2(cap, strbuf) do { \
    strbuf.buf.capacity=(cap); strbuf.buf.size=0; strbuf.buf.allocator.allocator=NULL; strbuf.buf.allocator.app_data=NULL; strbuf.bdy[0]=0; \
} while(0)

void strbuf_recalculate_size_as_cstr(strbuf_t** buf_ptr) {
    strbuf_t* buf = *buf_ptr;
    buf->size = (int)strlen(buf->cstr);
}

/**
 * @brief Pops N characters from buffer, at a location specified by index.
 * @param buf_ptr The address of a pointer to the buffer.
 * @param index The position within the buffer to pop at.
 * @param count Amount of characters to pop.
 * @return A view of the buffer contents.
 **********************************************************************************/
strview_t strbuf_pop_at_index(strbuf_t** buf_ptr, int index, int count) {
    if(buf_ptr && *buf_ptr) {
        strbuf_t* buf = *buf_ptr;
        if (index < buf->size) {
            count = _strbuf_int_min(buf->size - index, count);
            memmove(
                &buf->cstr[index],
                &buf->cstr[index + count],
                (size_t)(buf->size - (index + count) +1)); // +1 includes null terminator
            buf->size -= count;
        }
    }
    return buf_ptr ? strview_of_buf(*buf_ptr) : STRVIEW_INVALID;
}

/*
void strbuf_pop_at_index_TEST(void) {
    strbuf_t *line = strbuf_create_init(cstr(""), NULL);
    strbuf_assign(&line, cstr("Hello world!"));
    printf("ORIGINAL [%s]\n", line->cstr);

    strbuf_assign(&line, cstr("Hello world!"));
    strbuf_pop_at_index(&line, 0, 0);
    printf("[%s] size: %d\n", line->cstr, line->size);

    strbuf_assign(&line, cstr("Hello world!"));
    strbuf_pop_at_index(&line, 0, 1);
    printf("[%s] size: %d\n", line->cstr, line->size);

    strbuf_assign(&line, cstr("Hello world!"));
    strbuf_pop_at_index(&line, 0, 2);
    printf("[%s] size: %d\n", line->cstr, line->size);

    strbuf_assign(&line, cstr("Hello world!"));
    strbuf_pop_at_index(&line, 11, 4);
    printf("[%s] size: %d\n", line->cstr, line->size);

    strbuf_assign(&line, cstr("Hello world!"));
    strbuf_pop_at_index(&line, 6, 1);
    printf("[%s] size: %d\n", line->cstr, line->size);

    strbuf_assign(&line, cstr("Hello world!"));
    strbuf_pop_at_index(&line, 5, 100);
    printf("[%s] size: %d\n", line->cstr, line->size);

    strbuf_assign(&line, cstr("Hello world!"));
    strbuf_pop_at_index(&line, 0, 100000);
    printf("[%s] size: %d\n", line->cstr, line->size);
}
*/

#endif // !STRBUF_EXTRA_H
