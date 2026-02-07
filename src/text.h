#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int x;
    int y;
} GlyphEntry;

typedef struct {
    uint32_t  *atlas;
    int        atlas_width;
    int        atlas_height;
    int        glyph_width;
    int        glyph_height;
    GlyphEntry glyphs[128];
} GlyphCache;

bool glyph_cache_init(GlyphCache *cache, const char *font_bitmap_path,
                       int glyph_w, int glyph_h);
bool glyph_cache_init_generated(GlyphCache *cache, int glyph_w, int glyph_h);
void text_draw_char(const GlyphCache *cache, char c, int dest_x, int dest_y,
                    uint32_t color);
void text_draw_string(const GlyphCache *cache, const char *str,
                      int x, int y, uint32_t color);
int  text_string_width(const GlyphCache *cache, const char *str);

#endif // TEXT_H
