#include "graphics.h"
#include "vbe.h"
#include "memory.h"
#include "math.h" // For abs
#include "font.h" // Will be created next

void put_pixel(int x, int y, uint32_t color) {
    // Ensure we have valid screen info and the coordinates are within bounds
    if (!g_vbe_screen_info || x < 0 || y < 0 || x >= g_vbe_screen_info->width || y >= g_vbe_screen_info->height) {
        return;
    }

    // The framebuffer is a linear array of pixels
    uint32_t bytes_per_pixel = g_vbe_screen_info->bpp / 8;
    uint8_t* framebuffer = (uint8_t*)g_vbe_screen_info->physical_buffer;

    // Calculate the offset. Note the use of bytes_per_line (pitch)
    uint32_t offset = (y * g_vbe_screen_info->bytes_per_line) + (x * bytes_per_pixel);

    // Write the color value to the framebuffer memory
    // We assume a 32bpp mode, so we can cast to uint32_t*
    *(uint32_t*)(framebuffer + offset) = color;
}

void graphics_clear_screen(uint32_t color) {
    if (!g_vbe_screen_info) return;

    uint32_t bytes_per_pixel = g_vbe_screen_info->bpp / 8;
    uint32_t* framebuffer = (uint32_t*)g_vbe_screen_info->physical_buffer;
    uint32_t pitch_in_pixels = g_vbe_screen_info->bytes_per_line / bytes_per_pixel;

    // Fill the entire first scanline (up to the pitch) with the color.
    // This ensures we clear any padding bytes as well.
    for (uint32_t x = 0; x < pitch_in_pixels; x++) {
        framebuffer[x] = color;
    }

    // Now, copy the first line to all other lines. Start from the second line (y=1).
    for (uint16_t y = 1; y < g_vbe_screen_info->height; y++) {
        memcpy((void*)(framebuffer + y * pitch_in_pixels), (void*)framebuffer, g_vbe_screen_info->bytes_per_line);
    }
}

void graphics_draw_rect(int x, int y, int width, int height, uint32_t color) {
    if (!g_vbe_screen_info) return;

    // Clamp coordinates and dimensions to screen bounds
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + width;
    int y1 = y + height;
    if (x1 > g_vbe_screen_info->width) x1 = g_vbe_screen_info->width;
    if (y1 > g_vbe_screen_info->height) y1 = g_vbe_screen_info->height;

    int clipped_width = x1 - x0;
    int clipped_height = y1 - y0;

    if (clipped_width <= 0 || clipped_height <= 0) return;

    uint32_t bytes_per_pixel = g_vbe_screen_info->bpp / 8;
    uint32_t* framebuffer = (uint32_t*)g_vbe_screen_info->physical_buffer;
    uint32_t pitch_in_pixels = g_vbe_screen_info->bytes_per_line / bytes_per_pixel;

    // Get pointer to the start of the first line of the rectangle
    uint32_t* first_line = framebuffer + (y0 * pitch_in_pixels) + x0;

    // Draw the first line of the rectangle
    for (uint32_t i = 0; i < clipped_width; i++) {
        first_line[i] = color;
    }

    // Copy the first line to the subsequent lines of the rectangle
    for (int j = 1; j < clipped_height; j++) {
        memcpy((void*)(first_line + j * pitch_in_pixels), (void*)first_line, clipped_width * bytes_per_pixel);
    }
}

// A simple implementation of Bresenham's line algorithm
void graphics_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (int)fabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -(int)fabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void graphics_draw_char(int x, int y, char c, uint32_t color) {
    // Get the font data for the character
    const uint8_t* glyph = font8x8_basic[(uint8_t)c];

    // Draw the character pixel by pixel
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((glyph[i] >> j) & 1) {
                put_pixel(x + j, y + i, color);
            }
        }
    }
}

void graphics_draw_string(int x, int y, const char* str, uint32_t color) {
    int start_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 8; // Move to the next line
            x = start_x; // Reset x to the starting position
        } else {
            graphics_draw_char(x, y, *str, color);
            x += 8; // Move to the next character position
        }
        str++;
    }
}