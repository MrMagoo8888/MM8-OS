#include "framebuffer_text.h"
#include "framebuffer.h"
#include "fonts/font.h"
#include <stdint.h>

/* Helper to get pointer to LFB (assumes identity-mapped physical -> virtual) */
static inline uint8_t *fb_ptr(void) {
    FramebufferInfo *f = fb_info();
    return (uint8_t*)(uintptr_t)f->framebuffer_phys;
}

void fb_draw_glyph(int px, int py, uint32_t color, const uint8_t glyph[8]) {
    FramebufferInfo *f = fb_info();
    if (!f->framebuffer_phys) return;
    int bpp = (f->bpp + 7) / 8;
    uint8_t *fb = fb_ptr();
    for (int gy = 0; gy < 8; ++gy) {
        uint8_t row = glyph[gy];
        uint8_t *p = fb + (py + gy) * f->pitch + px * bpp;
        for (int gx = 0; gx < 8; ++gx) {
            if (row & (1 << (7 - gx))) {
                if (bpp == 4) {
                    *(uint32_t*)p = color;
                } else if (bpp == 3) {
                    p[0] = (color) & 0xFF;
                    p[1] = (color >> 8) & 0xFF;
                    p[2] = (color >> 16) & 0xFF;
                } else if (bpp == 2) {
                    *(uint16_t*)p = (uint16_t)color;
                }
            }
            p += bpp;
        }
    }
}

void fb_draw_char(int cx, int cy, char c, uint32_t color) {
    int px = cx * 8;
    int py = cy * 8;
    uint8_t glyph_index = (uint8_t)c;
    const uint8_t *glyph = font8x8_basic[glyph_index % 128];
    fb_draw_glyph(px, py, color, glyph);
}

void fb_draw_text(int cx, int cy, const char* s, uint32_t color) {
    int x = cx;
    while (*s) {
        fb_draw_char(x++, cy, *s++, color);
    }
}

void fb_draw_text_centered(int row, const char* s, uint32_t color) {
    FramebufferInfo *f = fb_info();
    if (!f->framebuffer_phys) return;
    int cols = f->width / 8;
    int len = 0;
    const char *p = s;
    while (*p++) len++;
    int start = (cols - len) / 2;
    if (start < 0) start = 0;
    fb_draw_text(start, row, s, color);
}