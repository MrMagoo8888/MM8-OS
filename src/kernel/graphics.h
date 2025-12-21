#pragma once

#include "stdint.h"
#include "stdbool.h"

#define COLOR_RED   0x00FF0000

extern bool g_DoubleBufferEnabled;

void draw_pixel(int x, int y, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);

void graphics_init_double_buffer();
void graphics_swap_buffer();
void graphics_clear_buffer(uint32_t color);
void graphics_set_double_buffering(bool enabled);