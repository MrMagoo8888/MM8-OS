#include "graphics.h"
#include "vbe.h"

void draw_pixel(int x, int y, uint32_t color)
{
    // Add bounds checking to prevent writing outside the framebuffer
    if (x < 0 || x >= vbe_screen.width || y < 0 || y >= vbe_screen.height)
    {
        return;
    }

    // We only support 32 bpp for simplicity
    if (vbe_screen.bpp != 32)
    {
        return;
    }

    uint32_t* framebuffer = (uint32_t*)vbe_screen.physical_buffer;
    uint32_t pitch_in_dwords = vbe_screen.pitch / 4;

    framebuffer[y * pitch_in_dwords + x] = color;
}