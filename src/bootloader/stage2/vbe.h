#pragma once

#include "stdint.h"

// void draw_pixel(int x, int y, uint32_t color);
void __attribute__((cdecl)) draw_pixel(int x, int y, uint32_t color);