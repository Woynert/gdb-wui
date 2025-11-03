#include "strbuf.h"

#define STRBUF_STATIC_INIT2(cap, strbuf) do { \
    strbuf.buf.capacity=(cap); strbuf.buf.size=0; strbuf.buf.allocator.allocator=NULL; strbuf.buf.allocator.app_data=NULL; strbuf.bdy[0]=0; \
} while(0)

void strbuf_recalculate_size_as_cstr(strbuf_t** buf_ptr) {
    strbuf_t* buf = *buf_ptr;
    buf->size = (int)strlen(buf->cstr);
}
