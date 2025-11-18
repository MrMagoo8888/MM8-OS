#include "vga_text.h"
#include "vbe.h"
#include "font.h"
#include "memory.h"

void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= vbe_screen.width || y < 0 || y >= vbe_screen.height) {
        return;
    }

    uint8_t* screen = (uint8_t*)vbe_screen.physical_buffer;
    uint32_t offset = y * vbe_screen.bytes_per_line + x * vbe_screen.bytes_per_pixel;
    
    // Assuming 32bpp or 24bpp where we can write a dword
    *(uint32_t*)(screen + offset) = color;
}

void vga_text_put_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    uint8_t* font_char = default_font[(uint8_t)c];
    int screen_x = x * FONT_WIDTH;
    int screen_y = y * FONT_HEIGHT;

    for (int i = 0; i < FONT_HEIGHT; i++) {
        for (int j = 0; j < FONT_WIDTH; j++) {
            if ((font_char[i] >> (7 - j)) & 1) {
                put_pixel(screen_x + j, screen_y + i, fg);
            } else {
                put_pixel(screen_x + j, screen_y + i, bg);
            }
        }
    }
}

void vga_text_clear(uint32_t bg) {
    uint32_t* framebuffer = (uint32_t*)vbe_screen.physical_buffer;
    uint32_t pitch_dwords = vbe_screen.bytes_per_line / 4;

    for (uint32_t y = 0; y < vbe_screen.height; y++) {
        for (uint32_t x = 0; x < vbe_screen.width; x++) {
            framebuffer[y * pitch_dwords + x] = bg;
        }
    }
}

void vga_text_scroll(uint32_t bg) {
    uint8_t* framebuffer = (uint8_t*)vbe_screen.physical_buffer;
    uint32_t scroll_height = vbe_screen.height - FONT_HEIGHT;
    
    // Move everything up by one character row
    memcpy(framebuffer, 
           framebuffer + (FONT_HEIGHT * vbe_screen.bytes_per_line), 
           scroll_height * vbe_screen.bytes_per_line);

    // Clear the last row
    uint8_t* last_row_start = framebuffer + scroll_height * vbe_screen.bytes_per_line;
    uint32_t clear_size = FONT_HEIGHT * vbe_screen.bytes_per_line;
    // This is less efficient than a proper memset32, but good enough for now.
    memset(last_row_start, 0, clear_size);
}

uint32_t vga_text_get_width() {
    return vbe_screen.width / FONT_WIDTH;
}

uint32_t vga_text_get_height() {
    return vbe_screen.height / FONT_HEIGHT;
}