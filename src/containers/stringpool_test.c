#include "portable_utils.h"
#include "stringpool.h"


void test1(void) {
    Stringpool strpool = { 0 };
    Stringpool_create(&strpool);

    int view_id;
    strview_t result;

    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL("This is me"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    printfd("\n\nAlright gonna print all now.");

    for (int i = 0; i < strpool.pairs.size; ++i) {
        Stringpool__Pair pair = strpool.pairs.items[i];

        result = (strview_t) {
            .data = strpool.buffer->cstr + pair.start,
            .size = pair.size
        };
        ASSERT(strview_is_valid(result));

        printfd("%d [%"PRIstr"]", i, PRIstrarg(result));
    }

    printfd("CLEARING EVERYTHING...");
    Stringpool_clear_preserving(&strpool);
    printfd("CONTINUING...");


    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL("Hello"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL(""));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL("This is me"));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    view_id = Stringpool_append(&strpool, cstr_SL("lkdafjdls;jfdaljdofvjdsofjal dkjflajd lfjladsjvfoajadasofjcvodasjfcojdaofjdaos fajodisfj aopdsuf9p8uf93q4u9cfjidjlfjadljfdsjf98aua4ajf4lkj2fcljdsoafu48u2fodjalf;j84279158jfkjdaskfjd mjf9 0sudf90ja odjf kldasfj dlsjaf 98quf qoljf ldjfqp8eq9jf eljf aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  "));
    ASSERT(view_id != -1);
    result = Stringpool_get(&strpool, view_id);
    ASSERT(strview_is_valid(result));
    printfd("id %d Got [%"PRIstr"]", view_id, PRIstrarg(result));

    printfd("\n\nAlright gonna print all now.");

    for (int i = 0; i < strpool.pairs.size; ++i) {
        Stringpool__Pair pair = strpool.pairs.items[i];

        result = (strview_t) {
            .data = strpool.buffer->cstr + pair.start,
            .size = pair.size
        };
        ASSERT(strview_is_valid(result));

        printfd("%d [%"PRIstr"]", i, PRIstrarg(result));
    }


    Stringpool_destroy(&strpool);
}


int main (void) {
	test1();
}
