#pragma once

#include "../common/stdint.h"
#include "../common/bootinfo.h"

// Declaration for our C-based pixel drawing function.
void VBE_draw_pixel(int x, int y, uint32_t color);
void VBE_Initialize(VBE_INFO* vbe_info);