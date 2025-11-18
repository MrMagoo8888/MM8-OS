#pragma once
#include "stdint.h"

void vga_text_put_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void vga_text_clear(uint32_t bg);
void vga_text_scroll(uint32_t bg);

uint32_t vga_text_get_width();
uint32_t vga_text_get_height();