#pragma once

#include "stdint.h"

// This structure must match the one in entry.asm
typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    uint8_t  bpp;
    uint32_t physical_buffer;
} __attribute__((packed)) VbeScreenInfo;

// This will be a pointer to the VBE info structure passed by the bootloader.
extern VbeScreenInfo* g_vbe_screen;