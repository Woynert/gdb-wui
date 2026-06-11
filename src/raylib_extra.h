#ifndef RAYLIB_EXTRA
#define RAYLIB_EXTRA

#include "raylib.h"
#include "strview.h"
#include "stdio.h"

typedef union Rect2 {
    struct {
        float x;
        float y;
        float width;
        float height;
    };
    struct {
        Vector2 pos;
        Vector2 size;
    };
    Rectangle rect;
} Rect2;

/* For drawing vertically invertex textures (i.e. BeginTextureMode) */
void DrawTextureRec_flipped (Texture2D texture, Rectangle source, Vector2 position, Color tint) {
    DrawTextureRec( texture, (Rectangle){
        source.x, (float)texture.height - source.height - source.y,
        source.width, -source.height
    },
    position, tint );
}

void DrawTexture_flipped(Texture2D texture, int posX, int posY, Color tint)
{
    DrawTextureRec(texture, (Rectangle) { (float)posX, (float)posY, (float)texture.width, (float)-texture.height }, (Vector2) { 0, 0 }, tint);
}

// Defaults to '�'.
#define CODEPOINT_NOT_FOUND 0xFFFD

/*
 * Extracted from rtext.c
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
    printf("cursor %d\n", cursor);

    //int cpSize = 0;
    //codepoint = GetCodepointNext_woy(buf, &cpSize, initial_cursor);
    //if (codepoint != 0) *codepoint_size = cpSize;

    //return codepoint;
    return GetCodepointNext_woy(&buf[cursor], codepoint_size, minimum_size-cursor+1);
}

/*
 * Extracted from rtext.c
 * Get index position for a unicode character on font, fallbacks to '�'
 */
int GetGlyphIndex_woy(Font font, int codepoint) {
    int index = 0;
    if (!IsFontValid(font)) return index;
    int fallbackIndex = 0;
    for (int i = 0; i < font.glyphCount; i++)
    {
        if (font.glyphs[i].value == CODEPOINT_NOT_FOUND) fallbackIndex = i;

        if (font.glyphs[i].value == codepoint)
        {
            index = i;
            break;
        }
    }
    if ((index == 0) && (font.glyphs[0].value != codepoint)) index = fallbackIndex;
    return index;
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

/* @returns byte cursor. */
int utf8_visually_nearest(const char *str, int size, int visual_char_count_target) {
    int count = 0;
    int codepoint_size = 0;
    for (int i = 0; i < size;) {
        if (count >= visual_char_count_target) {
            return i;
        }
        GetCodepointNext_woy(&str[i], &codepoint_size, size-i);
        i += codepoint_size;
        ++count;
    }
    return size;
}

/*
 * Extracted from rtext.c
 */
void DrawTextCodepoint_woy(Font font, int codepoint, Vector2 position, float fontSize, Color tint) {
    int index = GetGlyphIndex_woy(font, codepoint);
    float scaleFactor = fontSize/(float)font.baseSize;
    Rectangle dstRec = {
        position.x + (float)font.glyphs[index].offsetX*scaleFactor - (float)font.glyphPadding*scaleFactor,
        position.y + (float)font.glyphs[index].offsetY*scaleFactor - (float)font.glyphPadding*scaleFactor,
        (font.recs[index].width + 2.0f*(float)font.glyphPadding)*scaleFactor,
        (font.recs[index].height + 2.0f*(float)font.glyphPadding)*scaleFactor };
    Rectangle srcRec = {
        font.recs[index].x - (float)font.glyphPadding,
        font.recs[index].y - (float)font.glyphPadding,
        font.recs[index].width + 2.0f*(float)font.glyphPadding,
        font.recs[index].height + 2.0f*(float)font.glyphPadding };
    DrawTexturePro(font.texture, srcRec, dstRec, (Vector2){ 0, 0 }, 0.0f, tint);
}

/*
 * Extracted from rtext.c
 * @note: chars spacing is NOT proportional to fontSize.
 */
void DrawTextEx_strview(
    Font font, strview_t string, Vector2 position, float fontSize,
    float spacing, float textLineSpacing, Color tint
) {
    float textOffsetY = 0;
    float textOffsetX = 0.0f;
    float scaleFactor = fontSize/(float)font.baseSize; 

    for (int i = 0; i < string.size;)
    {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext_woy(
                &string.data[i], &codepointByteCount, string.size-i);

        if (codepoint == '\n') {
            textOffsetY += (fontSize + textLineSpacing);
            textOffsetX = 0.0f;
        } else {
            if ((codepoint != ' ') && (codepoint != '\t')) {
                DrawTextCodepoint_woy(
                    font, codepoint,
                    (Vector2){
                        position.x + textOffsetX,
                        position.y + textOffsetY
                    },
                    fontSize, tint);
            }

            int index = GetGlyphIndex_woy(font, codepoint);
            if (font.glyphs[index].advanceX == 0) {
                textOffsetX += ((float)font.recs[index].width*scaleFactor + spacing);
            } else {
                textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
            }
        }
        i += codepointByteCount;   // Move text bytes counter to next codepoint
    }
}

#endif // !RAYLIB_EXTRA
