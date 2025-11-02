#pragma once
#include <stdint.h>

/* Draw an 8x8 glyph at pixel coordinates (x,y) */
void fb_draw_glyph(int px, int py, uint32_t color, const uint8_t glyph[8]);

/* Draw single ASCII char at character cell (cx,cy) where each cell is 8x8 pixels */
void fb_draw_char(int cx, int cy, char c, uint32_t color);

/* Draw a null-terminated string starting at character cell (cx,cy) */
void fb_draw_text(int cx, int cy, const char* s, uint32_t color);

/* Draw centered text in terms of character rows */
void fb_draw_text_centered(int row, const char* s, uint32_t color);