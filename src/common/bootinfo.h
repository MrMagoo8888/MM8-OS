#pragma once

#include "stdint.h" // This now correctly points to the file in the same directory

// This structure must match the layout defined in bootinfo.inc and used by the bootloader
typedef struct {
    uint32_t vbe_physical_buffer;
    uint32_t vbe_pitch;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    // Add other fields from the bootloader's mode_info_block if needed
} __attribute__((packed)) VBE_INFO;

// The bootloader places the VBE_INFO structure at this well-known address.
#define BOOTINFO_ADDR 0x9000