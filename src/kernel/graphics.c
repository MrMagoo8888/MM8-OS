#include "graphics.h"
#include "vbe.h"
#include "heap.h"
#include "memory.h"

static uint32_t* g_BackBuffer = NULL;
static bool g_DoubleBufferEnabled = false;

static int abs(int n) {
    return (n < 0) ? -n : n;
}

void graphics_init_double_buffer() {
    if (!g_vbe_screen) return;
    
    // Allocate memory for the back buffer
    // Size = Height * Pitch (Pitch is bytes per line)
    size_t buffer_size = g_vbe_screen->height * g_vbe_screen->pitch;
    
    if (!g_BackBuffer) {
        g_BackBuffer = (uint32_t*)malloc(buffer_size);
        if (g_BackBuffer) {
            memset(g_BackBuffer, 0, buffer_size);
        }
    }
}

void graphics_set_double_buffering(bool enabled) {
    if (enabled && !g_BackBuffer) {
        graphics_init_double_buffer();
    }
    g_DoubleBufferEnabled = enabled && (g_BackBuffer != NULL);
}

void graphics_swap_buffer() {
    if (!g_BackBuffer || !g_vbe_screen) return;
    memcpy((void*)g_vbe_screen->physical_buffer, g_BackBuffer, g_vbe_screen->height * g_vbe_screen->pitch);
}

void graphics_clear_buffer(uint32_t color) {
    if (!g_vbe_screen) return;
    
    uint32_t* target;
    if (g_DoubleBufferEnabled && g_BackBuffer) {
        target = g_BackBuffer;
    } else {
        target = (uint32_t*)g_vbe_screen->physical_buffer;
    }
    
    // Optimization for black
    if (color == 0) {
        memset(target, 0, g_vbe_screen->height * g_vbe_screen->pitch);
        return;
    }

    // Fill manually (assuming 32bpp)
    size_t pixels = (g_vbe_screen->height * g_vbe_screen->pitch) / 4;
    for (size_t i = 0; i < pixels; i++) {
        target[i] = color;
    }
}

void draw_pixel(int x, int y, uint32_t color)
{
    // Add bounds checking to prevent writing outside the framebuffer
    if (!g_vbe_screen || x < 0 || x >= g_vbe_screen->width || y < 0 || y >= g_vbe_screen->height)
    {
        return;
    }

    // We only support 32 bpp for simplicity
    if (g_vbe_screen->bpp != 32)
    {
        return;
    }

    uint32_t* framebuffer;
    
    if (g_DoubleBufferEnabled && g_BackBuffer) {
        framebuffer = g_BackBuffer;
    } else {
        framebuffer = (uint32_t*)g_vbe_screen->physical_buffer;
    }

    uint32_t pitch_in_dwords = g_vbe_screen->pitch / 4;

    framebuffer[y * pitch_in_dwords + x] = color;
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    // Safety counter to prevent infinite loops if coordinates are wild
    int max_iter = 1000000;
    while (max_iter-- > 0) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { 
            err -= dy; x0 += sx; 
        }
        if (e2 < dy) { 
            err += dx; y0 += sy; 
        }
    }
}