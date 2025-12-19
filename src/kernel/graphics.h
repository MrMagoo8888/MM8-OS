#pragma once

#include "stdint.h"
#include "stdbool.h"

#define COLOR_RED   0x00FF0000

void draw_pixel(int x, int y, uint32_t color);

void graphics_init_double_buffer();
void graphics_swap_buffer();
void graphics_clear_buffer(uint32_t color);
void graphics_set_double_buffering(bool enabled);