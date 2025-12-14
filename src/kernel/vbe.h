#pragma once

#include <stdint.h>

// This structure must match the layout defined in bootinfo.inc
typedef struct {
    uint32_t vbe_physical_buffer;
    uint32_t vbe_pitch;
    uint16_t vbe_width;
    uint16_t vbe_height;
    uint8_t vbe_bpp;
} __attribute__((packed)) BootInfo;

// The bootloader places the BootInfo structure at this well-known address.
#define BOOTINFO_ADDR 0x9000

// Declaration for our C-based pixel drawing function.
void VBE_draw_pixel(int x, int y, uint32_t color);