#include "graphics.h"
#include "vbe.h"
#include "math.h" // For abs

void put_pixel(int x, int y, uint32_t color) {
    // Ensure we have valid screen info and the coordinates are within bounds
    if (!g_vbe_screen_info || x < 0 || y < 0 || x >= g_vbe_screen_info->width || y >= g_vbe_screen_info->height) {
        return;
    }

    // The framebuffer is a linear array of pixels
    uint8_t* framebuffer = (uint8_t*)g_vbe_screen_info->physical_buffer;

    // Calculate the offset. Note the use of bytes_per_line (pitch)
    uint32_t offset = (y * g_vbe_screen_info->bytes_per_line) + (x * g_vbe_screen_info->bytes_per_pixel);

    // Write the color value to the framebuffer memory
    // We assume a 32bpp mode, so we can cast to uint32_t*
    *(uint32_t*)(framebuffer + offset) = color;
}

void graphics_clear_screen(uint32_t color) {
    if (!g_vbe_screen_info) return;

    uint32_t* framebuffer = (uint32_t*)g_vbe_screen_info->physical_buffer;
    uint32_t total_pixels = g_vbe_screen_info->width * g_vbe_screen_info->height;

    // This is a simple but potentially slow way to clear.
    // A memset would be faster if the pitch matches width * bytes_per_pixel.
    for (uint32_t i = 0; i < total_pixels; i++) {
        framebuffer[i] = color;
    }
}

void graphics_draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            put_pixel(x + i, y + j, color);
        }
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