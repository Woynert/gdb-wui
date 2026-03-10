#ifndef RAYLIB_EXTRA
#define RAYLIB_EXTRA

#include "raylib.h"
#include "strview.h"

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
 * EXTRACTED FROM rtext.c
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
        int codepoint = GetCodepointNext(&string.data[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n') {
            textOffsetY += (fontSize + textLineSpacing);
            textOffsetX = 0.0f;
        } else {
            if ((codepoint != ' ') && (codepoint != '\t')) {
                DrawTextCodepoint(
                    font, codepoint,
                    (Vector2){
                        position.x + textOffsetX,
                        position.y + textOffsetY
                    },
                    fontSize, tint);
            }

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
