#include "graphics.h"
#include "vbe.h"
#include "heap.h"
#include "memory.h"

static uint32_t* g_BackBuffer = NULL;
static bool g_DoubleBufferEnabled = false;

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
    if (!g_BackBuffer || !g_vbe_screen) return;
    
    // Optimization for black
    if (color == 0) {
        memset(g_BackBuffer, 0, g_vbe_screen->height * g_vbe_screen->pitch);
        return;
    }

    // Fill manually (assuming 32bpp)
    size_t pixels = (g_vbe_screen->height * g_vbe_screen->pitch) / 4;
    for (size_t i = 0; i < pixels; i++) {
        g_BackBuffer[i] = color;
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