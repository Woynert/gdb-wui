#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "stdio.h"
#include "math.h"
#include "limits.h"

#include "raylib.h"
#include "raylib_extra.h"
#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"
#include "portable_utils.h"
#include "better_mouse_input.h"

#ifndef DYNA__TYPE_UINT
#define DYNA__TYPE_UINT
#define DYNA__TYPE uint
#include "containers/da.h"
#undef DYNA__TYPE
#endif

#ifndef DYNA__TYPE_INT
#define DYNA__TYPE_INT
#define DYNA__TYPE int
#include "containers/da.h"
#undef DYNA__TYPE
#endif

typedef struct Textedit_Change {
    bool is_insert;  // false == deletion
    int start;
    strbuf_t *buffer;
} Textedit_Change;

#define WRING__TYPE Textedit_Change
#define WRING__NAMESPACE TexteditRing
#include "containers/wring.h"
#undef WRING__TYPE
#undef WRING__NAMESPACE

typedef struct TexteditVisualLine {
    int start; /* byte id on source buffer */
    int end;   /* byte id on source buffer */
    int line;  /* line id on source buffer */
    int wrap;  /* visual line wrap count */
} TexteditVisualLine;

#define DYNA__TYPE TexteditVisualLine
#include "containers/da.h"
#undef DYNA__TYPE

#define TEXTEDIT_LOG_SIZE 256

typedef struct Textedit_Log {
    TexteditRing ring;
    uint cursor; // Relative to end: 0 means latest, 1 means latest-1.
} Textedit_Log;

typedef struct Textedit {
    bool enabled;
    int cursor;
    //bool wrap;
    int scroll;      // Represents real lines.
    int wrap_scroll; // Offset from wrapped real line.
    bool is_selecting;
    int selection_origin;
    strbuf_t *buffer; // grows as needed (it's cleared on close (manually))
    int_Dyna linebreaks; /* Todo: Mark linebreaks as dirty. */
    TexteditVisualLine_Dyna visualblocks; 
    int line_amount;
    Textedit_Log editlog;
    /* cached */
    int cursor_visual_line;
    int selection_visual_line;
    //int first_visual_line_wrap_levels;
    //int second_visual_line_wrap_levels;
    bool gofocus_cursor;
    int visualline_corresponding_to_scroll; // Matches a real line.
    double cursor_blink_timestamp_secs;
    bool must_rebuild_visual_blocks;
    bool show_line_numbers;
    bool editable;
} Textedit;

void textedit_init(Textedit *textedit) {
    *textedit = (Textedit) { 0 };
    textedit->buffer = strbuf_create(0, NULL);
    textedit->linebreaks = int_Dyna_create();
    textedit->visualblocks = TexteditVisualLine_Dyna_create();

    textedit->editlog.ring = TexteditRing_create(TEXTEDIT_LOG_SIZE);
    for (int i = 0; i < TEXTEDIT_LOG_SIZE; ++i) {
        textedit->editlog.ring.buffer[i].buffer = strbuf_create(0, NULL);
    }
}

void textedit_free(Textedit *textedit) {
    strbuf_destroy(&textedit->buffer);
    int_Dyna_free(&textedit->linebreaks);
    TexteditVisualLine_Dyna_free(&textedit->visualblocks);

    for (int i = 0; i < TEXTEDIT_LOG_SIZE; ++i) {
        strbuf_destroy(&textedit->editlog.ring.buffer[i].buffer);
    }
    TexteditRing_free(&textedit->editlog.ring);
}

// FIXME: This startindex doesn't work because we are not preserving previous
//        lines.
void textedit__update_linebreaks(Textedit *textedit, int startindex) {
    strview_t buffer = strbuf_view2(textedit->buffer);
    if (startindex >= buffer.size) { startindex = 0; };
    int_Dyna_clear_preserve(&textedit->linebreaks);

    int_Dyna_append(&textedit->linebreaks, -1);
    for (int i = startindex; i < textedit->buffer->size; ++i) {
        if (buffer.data[i] == '\n') {
            int_Dyna_append(&textedit->linebreaks, i);
        }
    }
    int_Dyna_append(&textedit->linebreaks, textedit->buffer->size);
    textedit->line_amount = (int)textedit->linebreaks.size -2;
}

void textedit_enable(Textedit *textedit, strview_t initial_buffer) {
    textedit->enabled = true;
    textedit->cursor = 0;
    textedit->scroll = 0;
    textedit->must_rebuild_visual_blocks = true;
    strbuf_assign(&textedit->buffer, initial_buffer);
    textedit__update_linebreaks(textedit, 0);
}

void textedit_disable(Textedit *textedit) {
    textedit->enabled = false;
}

void textedit_toggle_line_numbers(Textedit *textedit, bool value) {
    textedit->show_line_numbers = value;
}

void textedit_set_editable(Textedit *textedit, bool value) {
    textedit->editable = value;
}

// void textedit_get_buffer(Textedit *textedit) { }

bool textedit_is_unicode_in_range(int unicode) {
    return (unicode >= 32 && unicode <= 126)
        || (unicode >= 160 && unicode <= 255);

    // https://en.wikipedia.org/wiki/List_of_Unicode_characters
}

void textedit__scroll_down(Textedit *textedit) {
    /* We decide either to increase 'scroll' or 'wrap_scroll'. */
    if (textedit->visualblocks.size > 1) {
        TexteditVisualLine next_line =
        textedit->visualblocks.items[
            textedit->visualline_corresponding_to_scroll + textedit->wrap_scroll +1
        ];
        if (next_line.line == textedit->scroll) {
            ++textedit->wrap_scroll;
        } else if (textedit->scroll < textedit->line_amount -1) {
            textedit->visualline_corresponding_to_scroll += textedit->wrap_scroll +1;
            textedit->wrap_scroll = 0;
            ++textedit->scroll;
        }
    }
    textedit->must_rebuild_visual_blocks = true;
}

void textedit__scroll_up(Textedit *textedit) {
    /* We decide either to decrease 'scroll' or 'wrap_scroll'. */
    if (textedit->visualblocks.size > 1) {
        if (textedit->wrap_scroll > 0) {
            --textedit->wrap_scroll;
        } else if (textedit->scroll > 0) {
            TexteditVisualLine prev_line = textedit->visualblocks.items[
                textedit->visualline_corresponding_to_scroll + textedit->wrap_scroll -1 ];
            --textedit->scroll;
            textedit->visualline_corresponding_to_scroll -= prev_line.wrap +1;
            textedit->wrap_scroll = prev_line.wrap;
        }
    }
    textedit->must_rebuild_visual_blocks = true;
}

void textedit__cursor_down(Textedit *textedit) {
    if (textedit->cursor_visual_line == -1
        || textedit->cursor_visual_line +1 >= (int)textedit->visualblocks.size
        ) { return; }

    const TexteditVisualLine curr_line =
        textedit->visualblocks.items[textedit->cursor_visual_line];
    const TexteditVisualLine next_line =
        textedit->visualblocks.items[textedit->cursor_visual_line +1];

    textedit->cursor = int_min(
        next_line.start + textedit->cursor - curr_line.start,
        next_line.end -1
    );

    // For now: Snap to nearest codepoint to the left.
    // Ideally: It should snap to the visually nearest.
    // TODO: Wrap this magic numbers in a function or something.
    const char *buf = textedit->buffer->cstr;
    while (textedit->cursor >= 0) {
        if (((0x80 & buf[textedit->cursor]) == 0) ||
            ((0xc0 & buf[textedit->cursor]) != 0x80)) {
            break;
        }
        --textedit->cursor;
    }
}

void textedit__cursor_up(Textedit *textedit) {
    if (textedit->cursor_visual_line -1 < 0) { return; }

    const TexteditVisualLine curr_line =
        textedit->visualblocks.items[textedit->cursor_visual_line];
    const TexteditVisualLine prev_line =
        textedit->visualblocks.items[textedit->cursor_visual_line -1];

    textedit->cursor = int_min(
        prev_line.end -1,
        prev_line.start + textedit->cursor - curr_line.start
    );

    const char *buf = textedit->buffer->cstr;
    while (textedit->cursor >= 0) {
        if (((0x80 & buf[textedit->cursor]) == 0) ||
            ((0xc0 & buf[textedit->cursor]) != 0x80)) {
            break;
        }
        --textedit->cursor;
    }
}

void textedit__update_selection(Textedit *textedit, bool shift_pressed, int cursor_prev_pos) {
    // shift    + no_selecting -> start selecting
    // shift    + selecting    -> continue selecting
    // no shift + selecting    -> stop selecting
    if (shift_pressed) {
        if (!textedit->is_selecting) {
            textedit->selection_origin = cursor_prev_pos;
            textedit->is_selecting = cursor_prev_pos != textedit->cursor;
        }
        else if (textedit->selection_origin == textedit->cursor) {
            textedit->is_selecting = false;
        }
    }
    else if (textedit->is_selecting) {
        textedit->selection_origin = textedit->cursor;
        textedit->is_selecting = false;
    }
}

void textedit__discard_undo_log(Textedit *textedit) {
    for (uint i = 0; i < textedit->editlog.cursor; ++i) {
        TexteditRing_pop_head(&textedit->editlog.ring);
    }
    textedit->editlog.cursor = 0;
}

void textedit__move_by_world(Textedit *textedit, int dir) {
    strview_t delimiters = cstr("\n(){};.,\"\'#");
    int new_cursor;
    bool found_space = false;

    if (dir > 0) {
        new_cursor = textedit->buffer->size;
        for (int i = textedit->cursor+1; i < textedit->buffer->size; ++i) {
            if (textedit->buffer->cstr[i] == ' ') {
                found_space = true;
                continue;
            }
            for (int k = 0; k < delimiters.size; ++k) {
                if (textedit->buffer->cstr[i] == delimiters.data[k] || found_space) {
                    new_cursor = i;
                    goto exit_loop;
                }
            }
        }
    } else {
        // Only check for Char-Space diff when going LEFT like GTK.
        bool found_char = false; 
        new_cursor = 0;
        for (int i = textedit->cursor-2; i >= 0; --i) {
            if (textedit->buffer->cstr[i] == ' ') {
                if (found_char) {
                    new_cursor = i +1;
                    goto exit_loop;
                }
                found_space = true;
                continue;
            }
            for (int k = 0; k < delimiters.size; ++k) {
                if (textedit->buffer->cstr[i] == delimiters.data[k] || found_space) {
                    new_cursor = i +1;
                    goto exit_loop;
                }
            }
            found_char = true;
        }
    }
    exit_loop:
    textedit->cursor = new_cursor;
}

void textedit__apply_change(
    Textedit *textedit, const Textedit_Change *change, bool undo
) {
    // (undo ? !change->is_insert : change->is_insert) -> XOR.
    if (undo ^ change->is_insert) {
        strbuf_insert_at_index(
            &textedit->buffer, change->start, strbuf_view2(change->buffer));
        textedit->cursor = change->start + change->buffer->size;
    } else {
        strbuf_pop_at_index(
                &textedit->buffer, change->start, change->buffer->size);
        textedit->cursor = change->start;
    }
    textedit__update_linebreaks(textedit, 0);
    textedit->must_rebuild_visual_blocks = true;
    textedit->gofocus_cursor = true;
}

void textedit__insert(Textedit *textedit, int start, strview_t str) {
    // Discard undo history to create a 'new branch'.
    textedit__discard_undo_log(textedit);

    TexteditRing_extend_head_force(&textedit->editlog.ring);
    Textedit_Change *change = TexteditRing_get_head(&textedit->editlog.ring);
    if (change == NULL) { return; }

    change->is_insert = true;
    change->start = start;
    strbuf_assign(&change->buffer, str);
    strbuf_shrink(&change->buffer);
    textedit__apply_change(textedit, change, false);
}

void textedit__delete_chunk(Textedit *textedit, int start, int size) {
    textedit__discard_undo_log(textedit);
    TexteditRing_extend_head_force(&textedit->editlog.ring);
    Textedit_Change *change = TexteditRing_get_head(&textedit->editlog.ring);
    if (change == NULL) { return; }

    change->is_insert = false;
    change->start = start;
    strview_t chunk =
            strview_sub(strbuf_view2(textedit->buffer), start, start+size);
    strbuf_assign(&change->buffer, chunk);
    strbuf_shrink(&change->buffer);
    textedit__apply_change(textedit, change, false);
}

void textedit__delete_selection(Textedit *textedit) {
    int start, end;
    if (textedit->cursor > textedit->selection_origin) {
        start = textedit->selection_origin;
        end = textedit->cursor;
    } else {
        start = textedit->cursor;
        end = textedit->selection_origin;
    }
    if (start - end == 0) { return; }

    textedit->is_selecting = false;
    textedit__delete_chunk(textedit, start, end -start);
}

void textedit__select_all(Textedit *textedit) {
    if (textedit->buffer->size <= 0) {
        textedit->is_selecting = false;
        return;
    }
    textedit->is_selecting = true;
    textedit->selection_origin = 0;
    textedit->cursor = textedit->buffer->size;
}

/*
textedit_
woy_textedit_
wync_delta_
wyncDelta_
WyncDelta_
textedit_
Wyncdelta
*/

void textedit__copy_selection(Textedit *textedit) {
    int start, end;
    if (textedit->cursor > textedit->selection_origin) {
        start = textedit->selection_origin;
        end = textedit->cursor;
    } else {
        start = textedit->cursor;
        end = textedit->selection_origin;
    }
    if (start - end == 0) { return; }

    // SetClipboardText requires a c-string so to not allocate a gigantic
    // buffer we're gonna use the original buffer and add NULL where the
    // selection ends. Once finished we'll restore the original character.
    char og_char = ((char *)textedit->buffer->cstr)[end];
    (textedit->buffer->cstr)[end] = '\0';
    SetClipboardText(textedit->buffer->cstr + start);
    (textedit->buffer->cstr)[end] = og_char;
}

void textedit__build_visual_lines(
    Textedit *textedit, int view_columns, int from_real_line, int max_line_amount
){
    if (from_real_line >= (int)(textedit->linebreaks.size-1)) { return; }

    strview_t source = strbuf_view2(textedit->buffer);

    int initial_scroll = int_max(0, from_real_line -1);
    int b = textedit->linebreaks.items[initial_scroll] +1; // byte
    int chunk_start = b;
    int line_counter = 0;
    int wrap_counter = 0;
    int codepoint_count = 0;

    TexteditVisualLine_Dyna_clear_preserve(&textedit->visualblocks);

    for (;b <= source.size && line_counter < max_line_amount;) {

        int codepointByteCount = 0;
        int codepoint = GetCodepointNext_woy(
                &source.data[b], &codepointByteCount, source.size - b);

        // new line or EOF
        if (codepoint == '\n' || b == source.size) {
            TexteditVisualLine_Dyna_append(&textedit->visualblocks,
                (TexteditVisualLine) {
                    .start = chunk_start,
                    .end   = b +1,
                    .line = initial_scroll + line_counter,
                    .wrap = wrap_counter
                }
            );

            ++b; // skip codepoint
            chunk_start = b;
            codepoint_count = 0;
            ++line_counter;
            wrap_counter = 0;
            continue;
        }
        // max columns reached
        if (codepoint_count == view_columns) {
            TexteditVisualLine_Dyna_append(&textedit->visualblocks,
                (TexteditVisualLine) {
                    .start = chunk_start,
                    .end   = b,
                    .line = initial_scroll + line_counter,
                    .wrap = wrap_counter
                }
            );

            chunk_start = b;
            codepoint_count = 0;
            ++wrap_counter;
            continue;
        }
        ++codepoint_count;
        b += codepointByteCount;
    }
}

void textedit__find_cursor_visual_line(Textedit *textedit) {
    textedit->cursor_visual_line = -1;
    textedit->selection_visual_line = -1;
    for (int i = 0; i < (int)textedit->visualblocks.size; ++i) {
        TexteditVisualLine line = textedit->visualblocks.items[i];
        if (textedit->cursor >= line.start
            && textedit->cursor < line.end
        ) {
            textedit->cursor_visual_line = i;
        }
        if (textedit->selection_origin >= line.start
            && textedit->selection_origin < line.end
        ) {
            textedit->selection_visual_line = i;
        }
    }
}

/* NOTE: Maybe merge with above function? */
void textedit__find_visualline_corresponding_to_scroll(Textedit *textedit) {
    textedit->visualline_corresponding_to_scroll = -1;
    for (int i = 0; i < (int)textedit->visualblocks.size; ++i) {
        if (textedit->visualblocks.items[i].line == textedit->scroll) {
            textedit->visualline_corresponding_to_scroll = i;
            break;
        }
    }
}

/*
void textedit__debug_print_last_first_lines(Textedit *textedit, int view_max_lines) {
    int first_line_index = int_max(
        0,
        textedit->first_visual_line_wrap_levels - textedit->wrap_scroll
    );
    int last_line_index = int_min(
        (int)textedit->visualblocks.size -1,
        first_line_index + view_max_lines
    );
    TexteditVisualLine line = textedit->visualblocks.items[first_line_index];
    printf("%d first [%"PRIstr"]\n",
        first_line_index,
        PRIstrarg(
            strview_sub(strview_of_buf(textedit->buffer), line.start, line.end)
        )
    );
    line = textedit->visualblocks.items[last_line_index];
    printf("%d last  [%"PRIstr"]\n",
        last_line_index,
        PRIstrarg(
            strview_sub(strview_of_buf(textedit->buffer), line.start, line.end)
        )
    );
}
*/

void textedit__reset_cursor_blink(Textedit *textedit) {
    textedit->cursor_blink_timestamp_secs = GetTime();
}

void textedit__debug_draw(
    Textedit *textedit, Vector2 pos, int view_max_lines,
    Font font, float font_size, float font_spacing
) {
    if (1) { // DEBUG LINE RENDERING
        static strbuf_space_t(4096) _tmp_str = STRBUF_STATIC_INIT(4096);
        strbuf_t *tmp_str = (strbuf_t*)(&_tmp_str);

        float line_spacing = 2;
        float line_height = font_size + line_spacing;
        strview_t source = strbuf_view2(textedit->buffer);
        for (int i = 0; i < (int)textedit->visualblocks.size; ++i) {
            // Line
            TexteditVisualLine visual_line = textedit->visualblocks.items[i];
            strview_t lineview = strview_sub(source, visual_line.start, visual_line.end);
            DrawTextEx_strview( font,
                lineview,
                (Vector2) { pos.x, pos.y + (float)i * line_height },
                font_size, font_spacing, line_spacing, BLACK);

            // Extra info
            int cp_len = utf8_codepoint_count(lineview.data, lineview.size);
            strbuf_printf(&tmp_str, "i:%d size:%d cp_len:%d",
                i, visual_line.end - visual_line.start, cp_len
            );
            DrawTextEx_strview( font,
                strbuf_view(&tmp_str),
                (Vector2) { pos.x + 200, pos.y + (float)i * line_height },
                font_size, font_spacing, line_spacing, BLACK);

        }
        DrawRectangleLinesEx((Rectangle){
            pos.x, pos.y + (float)(textedit->visualline_corresponding_to_scroll)
                        * line_height,
            300, line_height }, 2, BLUE);
        DrawRectangleLinesEx((Rectangle){
            pos.x, pos.y + (float)(textedit->visualline_corresponding_to_scroll
                    + textedit->wrap_scroll) * line_height,
            300, line_height }, 2, RED);
        DrawRectangleLinesEx((Rectangle){
            pos.x, pos.y + (float)(int_min((int)textedit->visualblocks.size -1,
            textedit->visualline_corresponding_to_scroll + textedit->wrap_scroll
                + view_max_lines)) * line_height,
            300, line_height }, 2, PINK);
    }

    if (0) { // DEBUG EDIT HISTORY
        static strbuf_space_t(4096) _tmp_str = STRBUF_STATIC_INIT(4096);
        strbuf_t *tmp_str = (strbuf_t*)(&_tmp_str);
        static strbuf_space_t(4096) _tmp_str_bk = STRBUF_STATIC_INIT(4096);
        strbuf_t *tmp_str_bk = (strbuf_t*)(&_tmp_str_bk);

        TexteditRing *ring = &textedit->editlog.ring;
        uint actual_cursor = ring->size - textedit->editlog.cursor;
        strbuf_printf(&tmp_str, "History size %u, cursor %d, actual %d\n",
                ring->size, textedit->editlog.cursor, actual_cursor);

        for (uint i = 0; i < ring->size; ++i) {
            Textedit_Change *change = &ring->buffer[i];
            strbuf_append_printf(&tmp_str, "%s %d: %s at %d '%s'\n",
                i == actual_cursor ? "->" : "  ",
                i, change->is_insert ? "insert" : "delete",
                change->start, change->buffer->cstr);
        }

        if (strview_compare(strbuf_view(&tmp_str), strbuf_view(&tmp_str_bk)) != 0){
            printf("[\n%s\n]\n", tmp_str->cstr);
            strbuf_cat(&tmp_str_bk, strbuf_view(&tmp_str));
        }
    }

    if (0) { // DEBUG CODEPOINTS
        int posx = (int)pos.x;
        int posy = (int)pos.y + 300;
        DrawRectangle(posx, posy, font.texture.width, font.texture.height, BLACK);
        DrawTexture(font.texture, posx, posy, WHITE);
    }
}


void textedit_draw(
    Textedit *textedit, Rect2 view_rect,
    Font font, float font_size, float font_spacing, float font_width
) {
    //BeginTextureMode(GUI.aux_texture);
    DrawRectangleRec(view_rect.rect, GRAY);

    float line_spacing = 2;
    float line_height = font_size + line_spacing;
    float number_padding = 0;
    if (textedit->show_line_numbers) {
        number_padding = (float)(int_digit_places(textedit->line_amount) +1)
        * (font_width + font_spacing);
    }
    int view_columns = (int)floorf((view_rect.width - number_padding) /
                    (font_width + font_spacing));

    // scrolling

    if (GetMouseWheelMove() != 0) {
        int scroll_dir = GetMouseWheelMove() > 0 ? 1 : -1;

        if (scroll_dir < 0) {
            textedit__scroll_down(textedit);
        } else {
            textedit__scroll_up(textedit);
        }
    }

    /*
     * Some of these variables only need to be updated on cursor change
     */

    strview_t source = strbuf_view2(textedit->buffer);
    float fontSize = font_size;
    float textLineSpacing = line_spacing;
    TexteditVisualLine visual_line = { 0 };

    int view_max_lines = (int)floorf(view_rect.height / line_height);

    if (textedit->must_rebuild_visual_blocks) {
        textedit->must_rebuild_visual_blocks = false;
        textedit__build_visual_lines(
            textedit, view_columns,
            textedit->scroll - view_max_lines,
            view_max_lines * 2 + 2
        );
    }

    /* Get cursor visual line */

    textedit__find_cursor_visual_line(textedit);

    /* Find visualline_corresponding_to_scroll */

    textedit__find_visualline_corresponding_to_scroll(textedit);
    int visualline_start = textedit->visualline_corresponding_to_scroll
                     + textedit->wrap_scroll;
    int visualline_end = int_min(
        visualline_start + view_max_lines, (int)textedit->visualblocks.size -1
    );
    visualline_start = int_clamp(0, (int)textedit->visualblocks.size -1, visualline_start);
    visualline_end   = int_clamp(0, (int)textedit->visualblocks.size -1, visualline_end);

    //printf("%d:%d\n", textedit->cursor_visual_line, textedit->selection_visual_line);

    //printf("%d:%d:%d\n",
        //textedit->scroll,
        //textedit->wrap_scroll,
        //textedit->visualline_corresponding_to_scroll
    //);

    /* Draw selection area */

    do {
    if (textedit->is_selecting && textedit->visualblocks.size > 0) {
        int cursor_start, cursor_end, visual_start, visual_end;
        if (textedit->cursor > textedit->selection_origin) {
            cursor_start = textedit->selection_origin;
            visual_start = textedit->selection_visual_line;
            cursor_end = textedit->cursor;
            visual_end = textedit->cursor_visual_line;
        } else {
            cursor_start = textedit->cursor;
            visual_start = textedit->cursor_visual_line;
            cursor_end = textedit->selection_origin;
            visual_end = textedit->selection_visual_line;
        }
        if ((visual_start == -1 || visual_start < visualline_start) &&
            (visual_end == -1 || visual_end > visualline_end) // ends not visible
        ) {
            TexteditVisualLine line = textedit->visualblocks.items[visualline_start];
            if (!(cursor_start <= line.start && line.start < cursor_end)) {
                break;
            }
        }
        if (visual_start == -1 || visual_start < visualline_start) { // start not visible
            visual_start = visualline_start;
            TexteditVisualLine line = textedit->visualblocks.items[visual_start];
            cursor_start = line.start;
        }
        if (visual_end == -1 || visual_end > visualline_end) { // end not visible
            visual_end = visualline_end;
            TexteditVisualLine line = textedit->visualblocks.items[visual_end];
            cursor_end = line.start + view_columns;
        }

        Rectangle rect;
        TexteditVisualLine line;
        int chars_x, lines_y;

        line = textedit->visualblocks.items[visual_start];
        lines_y = visual_start - visualline_start;
        chars_x = utf8_codepoint_count(
            &textedit->buffer->cstr[line.start], cursor_start - line.start);

        float start_x = (float)(chars_x)
            * (font_width + font_spacing) + view_rect.x + number_padding;
        float start_y = (float)(lines_y)
            * (font_size + line_spacing) + view_rect.y;

        line = textedit->visualblocks.items[visual_end];
        lines_y = visual_end - visualline_start;
        chars_x = utf8_codepoint_count(
            &textedit->buffer->cstr[line.start], cursor_end - line.start);

        float end_x = (float)(chars_x)
            * (font_width + font_spacing) + view_rect.x + number_padding;
        float end_y = (float)(lines_y)
            * (font_size + line_spacing) + view_rect.y;

        if (visual_start == visual_end) { 
            /* selection starts and end on the same line */
            rect = (Rectangle) {
                start_x,
                start_y,
                end_x - start_x, font_size
            };
            DrawRectangleRec(rect, BLUE);
        }
        else {
            rect = (Rectangle) {
                start_x,
                start_y,
                view_rect.x + view_rect.width - start_x, font_size
            };
            DrawRectangleRec(rect, BLUE); // start
            rect = (Rectangle) {
                view_rect.x + number_padding,
                end_y,
                end_x - (view_rect.x + number_padding), font_size
            };
            DrawRectangleRec(rect, BLUE); // end
            for (int i = 1; i < (visual_end - visual_start); ++i) {
                rect = (Rectangle) {
                    view_rect.x + number_padding,
                    start_y + (float)(i) * (font_size + line_spacing),
                    view_rect.width - number_padding, font_size
                };
                DrawRectangleRec(rect, BLUE); // body
            }
        }

        // draw selection end
        if (textedit->selection_visual_line < 0) { break; }
        line = textedit->visualblocks.items[textedit->selection_visual_line];
        lines_y = textedit->selection_visual_line - visualline_start;
        //chars_x = textedit->selection_origin - line.start;
        chars_x = utf8_codepoint_count(
                &textedit->buffer->cstr[line.start],
                textedit->selection_origin - line.start);
        start_x = view_rect.x + number_padding +
            (float)(chars_x) * (font_width + font_spacing) -1;
        start_y = view_rect.y +
            (float)(lines_y) * (font_size + line_spacing);
        DrawRectangleRec((Rectangle) { start_x, start_y, 2, font_size }, RED);
    }
    } while(0);

    // draw buffer

    static strbuf_space_t(16) _aux_str = STRBUF_STATIC_INIT(16);
    strbuf_t *aux_str = (strbuf_t*)(&_aux_str);

    //int visualline_end   = visualline_start + view_max_lines;
    //visualline_start = int_clamp(0, (int)textedit->visualblocks.size -1, visualline_start);
    //visualline_end   = int_clamp(0, (int)textedit->visualblocks.size -1, visualline_end);

    for (int i = visualline_start; i <= visualline_end; ++i) {
        visual_line = textedit->visualblocks.items[i];
        DrawTextEx_strview(
            font,
            strview_sub(source, visual_line.start,
                                visual_line.end),
            (Vector2) {
                view_rect.x + number_padding,
                view_rect.y + (float)(i - visualline_start) * (fontSize + textLineSpacing)
            },
            font_size, font_spacing, line_spacing, BLACK);

        if (visual_line.wrap != 0) continue;

        // draw numbers

        if (!textedit->show_line_numbers) continue;

        strbuf_printf(&aux_str, "%d", visual_line.line);

        DrawTextEx_strview(
            font,
            strbuf_view2(aux_str),
            (Vector2) {
                view_rect.x,
                view_rect.y + (float)(i - visualline_start) * (fontSize + textLineSpacing)
            },
            font_size, font_spacing, line_spacing, BLACK);
    }

    /* Draw cursor + blink */
    /* TODO: Unify with selection cursor logic */

    double time_since = GetTime() - textedit->cursor_blink_timestamp_secs;
    if (time_since > 1) { textedit__reset_cursor_blink(textedit); }

    if ((textedit->cursor_visual_line >= 0 && time_since <= 0.8) || 
        (!textedit->editable)
    ) {
        const TexteditVisualLine curr_line =
            textedit->visualblocks.items[textedit->cursor_visual_line];
        //const strview_t view_line = strview_sub(
            //strbuf_view(&textedit->buffer), curr_line.start, curr_line.end);

        int view_cursor_y = textedit->cursor_visual_line - visualline_start;
        //int view_cursor_x = utf8_codepoint_count(
                //view_line.data, textedit->cursor - curr_line.start);
        int view_cursor_x = utf8_codepoint_count(
            &textedit->buffer->cstr[curr_line.start],
            textedit->cursor - curr_line.start);

        DrawRectangleRec((Rectangle) {
            view_rect.x + number_padding +
            (float)(view_cursor_x) * (font_width + font_spacing) -1,
            view_rect.y +
            (float)(view_cursor_y) * (font_size + line_spacing) -2,
            2, font_size +4 }, WHITE);

    }
    DrawRectangleLinesEx(view_rect.rect, 1, BLACK);
    //EndTextureMode();

    // Debug drawing.

    textedit__debug_draw(
        textedit,
        (Vector2){view_rect.x +view_rect.width, view_rect.y},
        view_max_lines, font, font_size, font_spacing);

    // controls

    int prev_cursor;
    if (textedit->editable) {
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        if (IsKeyPressed(KEY_LEFT)) {
            prev_cursor = textedit->cursor;
            if (textedit->cursor > 0) {
                if (control) { textedit__move_by_world(textedit, -1); }
                else {
                    int codepoint_size = 0;
                    //strview_t view = strview_sub(
                        //strbuf_view(&textedit->buffer), 0, textedit->cursor);
                    int codepoint = GetCodepointPrev_woy(
                        textedit->buffer->cstr, &codepoint_size, textedit->cursor-1);
                    //int codepoint = GetCodepointPrevious(view.data, &codepoint_size);
                    //--textedit->cursor;
                    textedit->cursor -= codepoint_size;
                    printf("going left %d\n", codepoint);
                }
            }

            textedit__update_selection(textedit, shift, prev_cursor);
            textedit->gofocus_cursor = true;
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            prev_cursor = textedit->cursor;
            if (textedit->cursor < textedit->buffer->size) {
                if (control) { textedit__move_by_world(textedit, +1); }
                else {
                    //textedit->buffer
                    int codepoint_size = 0;
                    strview_t view = strview_sub(
                        strbuf_view(&textedit->buffer), textedit->cursor, INT_MAX);
                    printf("cursor %d\n", textedit->cursor);
                    int codepoint = GetCodepointNext_woy(view.data, &codepoint_size, view.size);
                    //++textedit->cursor;
                    textedit->cursor += codepoint_size;
                    printf("going right %d\n", codepoint);
                }
            }

            textedit__update_selection(textedit, shift, prev_cursor);
            textedit->gofocus_cursor = true;
        }

        if (IsKeyPressed(KEY_DOWN)
        ) {
            prev_cursor = textedit->cursor;
            textedit__cursor_down(textedit);
            textedit__update_selection(textedit, shift, prev_cursor);
            textedit->gofocus_cursor = true;
        }

        if (IsKeyPressed(KEY_UP)) {
            prev_cursor = textedit->cursor;
            textedit__cursor_up(textedit);
            textedit__update_selection(textedit, shift, prev_cursor);
            textedit->gofocus_cursor = true;
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (textedit->is_selecting) {
                textedit__delete_selection(textedit);
            } else if (textedit->cursor > 0) {
                textedit__delete_chunk(textedit, textedit->cursor-1, 1);
            }
            textedit->is_selecting = false;
        }

        char new_char[2] = { '\0', '\0' };
        int codepoint;
        while ((codepoint = GetCharPressed())) // write text
        {
            if (!textedit_is_unicode_in_range(codepoint)) continue;

            if (textedit->is_selecting) {
                textedit__delete_selection(textedit);
            }

            new_char[0] = (char)codepoint;
            textedit__insert(textedit, textedit->cursor, cstr(new_char));
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (textedit->is_selecting) {
                textedit__delete_selection(textedit);
            }

            new_char[0] = (char)'\n';
            textedit__insert(textedit, textedit->cursor, cstr(new_char));
            return;
        }

        // copy
        if (control && IsKeyPressed(KEY_C) && textedit->is_selecting) {
            textedit__copy_selection(textedit);
        }
        // cut
        if (control && IsKeyPressed(KEY_X) && textedit->is_selecting) {
            textedit__copy_selection(textedit);
            textedit__delete_selection(textedit);
        }
        // paste
        if (control && IsKeyPressed(KEY_V)) {
            const char *clipboard_cstr = GetClipboardText();
            strview_t clipboard = cstr(clipboard_cstr);
            if (clipboard.size > 0) {
                if (textedit->is_selecting) {
                    textedit__delete_selection(textedit);
                }

                textedit__insert(textedit, textedit->cursor, clipboard);
            }
        }
        // select all
        if (control && IsKeyPressed(KEY_A)) {
            textedit__select_all(textedit);
        }
        // redo
        if (control && shift && IsKeyPressed(KEY_Z)) {
            Textedit_Change *change = TexteditRing_get_from_head(
                    &textedit->editlog.ring,
                    textedit->editlog.cursor -1);
            if (change != NULL) {
                textedit__apply_change(textedit, change, false);
                --textedit->editlog.cursor;
                textedit->is_selecting = false;
            }
        }
        // undo
        else if (control && IsKeyPressed(KEY_Z)) {
            Textedit_Change *change = TexteditRing_get_from_head(
                    &textedit->editlog.ring,
                    textedit->editlog.cursor);
            if (change != NULL) {
                textedit__apply_change(textedit, change, true);
                ++textedit->editlog.cursor;
                textedit->is_selecting = false;
            }
        }
    }

    // mouse
    if (BetterMouse_is_held(MOUSE_BUTTON_LEFT)) {
        bool first_press = BetterMouse_is_pressed(MOUSE_BUTTON_LEFT);
        do {
            prev_cursor = textedit->cursor;

            Vector2 mouse_pos = GetMousePosition();
            float char_width = font_width + font_spacing;
            float mouse_x = mouse_pos.x - (view_rect.x + number_padding - char_width/2.f);
            float mouse_y = mouse_pos.y - (view_rect.y);
            int mouse_x_char_target = (int)floorf(mouse_x / char_width);
            int mouse_y_char = (int)floorf(mouse_y / line_height);

            //if (mouse_y_char < 0 || mouse_y_char > view_max_lines
                //|| mouse_x_char < 0 || mouse_x_char > view_columns) {
                //break;
            //}

            if (mouse_y_char < 0 || mouse_y_char > view_max_lines) {
                break;
            }

            int line_index = 
                textedit->visualline_corresponding_to_scroll
                + textedit->wrap_scroll
                + mouse_y_char;
            line_index = int_clamp(0, (int)textedit->visualblocks.size-1, line_index);
            TexteditVisualLine line = textedit->visualblocks.items[line_index];


            int mouse_x_char = utf8_visually_nearest(
                &textedit->buffer->cstr[line.start], line.end - line.start, mouse_x_char_target);

            textedit->cursor = int_clamp(0, line.end, line.start + mouse_x_char);
                //int_max(0, int_min(
                //line.end -1,
                //line.start + mouse_x_char
            //));

            textedit__reset_cursor_blink(textedit);
            textedit__update_selection(textedit, !first_press, prev_cursor);

        } while (0);
    }

    // scroll to cursor
    if (textedit->gofocus_cursor) {
        textedit->gofocus_cursor = false;

        textedit__reset_cursor_blink(textedit);

        // Do nothing if cursor is visible.

        textedit__find_cursor_visual_line(textedit);
        int first_line_index = int_max(
            0,
            textedit->visualline_corresponding_to_scroll + textedit->wrap_scroll
        );
        int last_line_index = int_min(
            (int)textedit->visualblocks.size -1,
            textedit->visualline_corresponding_to_scroll + textedit->wrap_scroll
            + view_max_lines
        );

        if (textedit->cursor_visual_line >= first_line_index
            && textedit->cursor_visual_line <= last_line_index) {
            return;
        }

        // Get cursor line + direction

        int cursor_line = -1;
        for (int i = 0; i < textedit->linebreaks.size; ++i) {
            if (textedit->cursor > (int)textedit->linebreaks.items[i]) {
                cursor_line = i-1;
            } else { break; }
        }
        ++cursor_line;

        TexteditVisualLine line = textedit->visualblocks.items[first_line_index];
        int dir = textedit->cursor > line.start ? 1 : -1;

        // Rebuild.

        textedit->scroll = cursor_line;
        textedit->wrap_scroll = 0;

        textedit__build_visual_lines(
            textedit, view_columns,
            textedit->scroll - view_max_lines,
            view_max_lines * 2
        );
        textedit__find_cursor_visual_line(textedit);
        textedit__find_visualline_corresponding_to_scroll(textedit);

        // Scroll until visible.

        first_line_index = int_max(
            0,
            textedit->visualline_corresponding_to_scroll + textedit->wrap_scroll
        );
        last_line_index = first_line_index + view_max_lines;
        // Warning: ^ Do not use to index, it's not capped.

        int lines_to_scroll =
            dir > 0 ? last_line_index - textedit->cursor_visual_line
                    : textedit->cursor_visual_line - first_line_index;

        for (int i = 0; i < abs(lines_to_scroll); ++i) {
            if (dir > 0) {
                if (lines_to_scroll > 0) { textedit__scroll_up(textedit);
                } else { textedit__scroll_down(textedit); }
            }
            else {
                if (lines_to_scroll > 0) { textedit__scroll_down(textedit);
                } else { textedit__scroll_up(textedit); }
            }
        }
    }


}

#endif // !TEXTEDIT_H
