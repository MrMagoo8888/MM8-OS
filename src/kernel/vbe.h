#pragma once

#include "stdint.h"

// This structure must match the 'vbe_screen' structure in entry.asm
typedef struct __attribute__((packed)) {
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;
    uint32_t bytes_per_pixel; // This was missing
    uint16_t bytes_per_line;  // This was the wrong size (pitch)
    uint32_t physical_buffer;
} vbe_screen_t;

extern vbe_screen_t* g_vbe_screen_info;