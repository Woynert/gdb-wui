#ifndef RAYLIB_EXTRA
#define RAYLIB_EXTRA

#include "raylib.h"
#include "strview.h"
#include "stdio.h"
#include "strbuf_extra.h"


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

/*
 * Extracted from rtext.c
 * Get index position for a unicode character on font, fallbacks to '�'
 */
int GetGlyphIndex_woy(Font font, int codepoint) {
    /* Defaults to '�' (0xFFFD). */

    int index = 0;
    if (!IsFontValid(font)) return index;
    int fallbackIndex = 0;
    for (int i = 0; i < font.glyphCount; i++)
    {
        if (font.glyphs[i].value == 0xFFFD) fallbackIndex = i;

        if (font.glyphs[i].value == codepoint)
        {
            index = i;
            break;
        }
    }
    if ((index == 0) && (font.glyphs[0].value != codepoint)) index = fallbackIndex;
    return index;
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
    Font font, const strview_t string, Vector2 position, float fontSize,
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
