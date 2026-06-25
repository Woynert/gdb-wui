#ifndef STRBUF_EXTRA_H
#define STRBUF_EXTRA_H

#include "strbuf.h"

static inline int _strbuf_int_min(int a, int b) { return a < b ? a : b; }
static strview_t strbuf_view2(strbuf_t* buf) // TODO: Delete this or justify it.
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
    (strbuf).buf.capacity=(cap); (strbuf).buf.size=0; (strbuf).buf.allocator.allocator=NULL; (strbuf).buf.allocator.app_data=NULL; (strbuf).bdy[0]=0; \
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
    return buf_ptr ? strbuf_view2(*buf_ptr) : STRVIEW_INVALID;
}

// Defaults to '�'.
#define CODEPOINT_NOT_FOUND 0xFFFD

/*
 * Extracted from raylib rtext.c
 * Fixes out of bounds read.
 */
int GetCodepointNext_woy(const char *text, int *codepointSize, int available_bytes) {
    const char *ptr = text;
    int codepoint = CODEPOINT_NOT_FOUND;
    *codepointSize = 1;
    if (text == NULL || available_bytes == 0) return codepoint;
    if (0xf0 == (0xf8 & ptr[0]) && available_bytes >= 4) {
        if (((ptr[1] & 0xC0) ^ 0x80) || ((ptr[2] & 0xC0) ^ 0x80) ||
            ((ptr[3] & 0xC0) ^ 0x80)) { return codepoint; }
        codepoint = ((0x07 & ptr[0]) << 18) | ((0x3f & ptr[1]) << 12)
            | ((0x3f & ptr[2]) << 6) | (0x3f & ptr[3]);
        *codepointSize = 4;
    }
    else if (0xe0 == (0xf0 & ptr[0]) && available_bytes >= 3) {
        if (((ptr[1] & 0xC0) ^ 0x80) || ((ptr[2] & 0xC0) ^ 0x80)) { return codepoint; }
        codepoint = ((0x0f & ptr[0]) << 12) | ((0x3f & ptr[1]) << 6) | (0x3f & ptr[2]);
        *codepointSize = 3;
    }
    else if (0xc0 == (0xe0 & ptr[0]) && available_bytes >= 2) {
        if ((ptr[1] & 0xC0) ^ 0x80) { return codepoint; }
        codepoint = ((0x1f & ptr[0]) << 6) | (0x3f & ptr[1]);
        *codepointSize = 2;
    }
    else if (0x00 == (0x80 & ptr[0])) {
        codepoint = ptr[0];
        *codepointSize = 1;
    }
    return codepoint;
}

/*
 * Extracted from rtext.c
 * @note Fixes out of bounds read.
 */
int GetCodepointPrev_woy(const char *buf, int *codepoint_size, int cursor)
{
    //const char *ptr = text;
    int minimum_size = cursor;
    int codepoint = CODEPOINT_NOT_FOUND;
    *codepoint_size = 1;
    if (buf == NULL) return codepoint;

    // Move to previous codepoint
    //do ptr--;
    //while (((0x80 & ptr[0]) != 0) && ((0xc0 & ptr[0]) ==  0x80));

    while (cursor >= 0) {
        //if (((0x80 & buf[cursor]) != 0) && ((0xc0 & buf[cursor]) ==  0x80)) {
        if (((0x80 & buf[cursor]) == 0) ||
            ((0xc0 & buf[cursor]) != 0x80)) {
            break;
        }
        --cursor;
    }

    //int cpSize = 0;
    //codepoint = GetCodepointNext_woy(buf, &cpSize, initial_cursor);
    //if (codepoint != 0) *codepoint_size = cpSize;

    //return codepoint;
    return GetCodepointNext_woy(&buf[cursor], codepoint_size, minimum_size-cursor+1);
}

/* @returns Codepoint count */
int utf8_codepoint_count(const char *str, int size) {
    int count = 0;
    int codepoint_size = 0;
    for (int i = 0; i < size;) {
        GetCodepointNext_woy(&str[i], &codepoint_size, size-i);
        i += codepoint_size;
        ++count;
    }
    return count;
}

/* @returns Codepoint count */
int utf8_codepoint_count_strview(const strview_t view) {
    return utf8_codepoint_count(view.data, view.size);
}

static void strbuf_empty(strbuf_t** buf_ptr)
{
    strbuf_t* buf = NULL;
    if (buf_ptr && *buf_ptr) {
        buf = *buf_ptr;
        buf->size = 0;
        buf->cstr[0] = 0;
    }
}


bool strview_equal(strview_t str1, strview_t str2)
{
    return strview_compare(str1, str2) == 0;
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
