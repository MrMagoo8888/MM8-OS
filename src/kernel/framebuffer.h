#pragma once
#include <stdint.h>
#include <stddef.h>

/* fallback for environments where uintptr_t isn't defined in stdint.h */
#ifndef UINTPTR_MAX
typedef unsigned long uintptr_t;
#endif

#define FRAMEBUFFER_INFO_ADDR 0x9000

typedef struct {
    uint32_t framebuffer_phys; /* dword */
    uint32_t width;            /* dword */
    uint32_t height;           /* dword */
    uint32_t pitch;            /* dword - bytes per scanline */
    uint8_t  bpp;              /* byte */
    uint8_t  _pad[3];
} FramebufferInfo;

static inline FramebufferInfo* fb_info(void) {
    return (FramebufferInfo*)(uintptr_t)FRAMEBUFFER_INFO_ADDR;
}

/* NOTE: This assumes the physical framebuffer is identity-mapped
   into the kernel address space. If you use paging, map framebuffer_phys. */

static inline void fb_putpixel(int x, int y, uint32_t color) {
    FramebufferInfo *f = fb_info();
    if (!f->framebuffer_phys) return;
    if ((uint32_t)x >= f->width || (uint32_t)y >= f->height) return;
    uint8_t *fb = (uint8_t*)(uintptr_t)f->framebuffer_phys;
    uint32_t bytes_per_pixel = f->bpp / 8;
    uint8_t *p = fb + y * f->pitch + x * bytes_per_pixel;
    if (bytes_per_pixel == 4) {
        *(uint32_t*)p = color;
    } else if (bytes_per_pixel == 3) {
        p[0] = (color      ) & 0xFF;
        p[1] = (color >> 8 ) & 0xFF;
        p[2] = (color >> 16) & 0xFF;
    } else if (bytes_per_pixel == 2) {
        *(uint16_t*)p = (uint16_t)color;
    }
}

static inline void fb_clear(uint32_t color) {
    FramebufferInfo *f = fb_info();
    if (!f->framebuffer_phys) return;
    uint8_t *fb = (uint8_t*)(uintptr_t)f->framebuffer_phys;
    uint32_t bytes_per_pixel = f->bpp / 8;
    uint32_t pixels_per_row = f->pitch / bytes_per_pixel;
    for (uint32_t y = 0; y < f->height; ++y) {
        uint32_t *row = (uint32_t*)(fb + y * f->pitch);
        for (uint32_t x = 0; x < pixels_per_row; ++x) {
            if (bytes_per_pixel == 4)
                row[x] = color;
            else {
                /* fallback per-pixel write for other bpp */
                fb_putpixel((int)x, (int)y, color);
            }
        }
    }
}