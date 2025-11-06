#pragma once

#include "stdint.h"
#include "vbe.h"

void put_pixel(int x, int y, uint32_t color);
void graphics_clear_screen(uint32_t color);
void graphics_draw_rect(int x, int y, int width, int height, uint32_t color);
void graphics_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void graphics_draw_char(int x, int y, char c, uint32_t color);
void graphics_draw_string(int x, int y, const char* str, uint32_t color);