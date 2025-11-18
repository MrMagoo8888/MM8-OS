#pragma once
#include "stdint.h"

// This struct must match the layout of 'vbe_screen' in entry.asm
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;
    uint32_t bytes_per_pixel;
    uint16_t bytes_per_line; // pitch
    uint32_t physical_buffer;
} __attribute__((packed)) VBE_ScreenInfo;

// This is defined in entry.asm and holds the current screen info
extern VBE_ScreenInfo vbe_screen;