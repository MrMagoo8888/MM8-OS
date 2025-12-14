#include "vbe.h"

void VBE_draw_pixel(int x, int y, uint32_t color)
{
    // Get a pointer to the BootInfo structure.
    BootInfo* bootInfo = (BootInfo*)BOOTINFO_ADDR;

    // Ensure we don't draw outside the screen boundaries.
    if (x < 0 || y < 0 || x >= bootInfo->vbe_width || y >= bootInfo->vbe_height) {
        return;
    }

    // Calculate the linear address of the pixel.
    // The framebuffer is a 1D array of pixels.
    // The address is: (y * pitch) + (x * bytes_per_pixel)
    uint8_t* framebuffer = (uint8_t*)bootInfo->vbe_physical_buffer;
    uint32_t* pixel_ptr = (uint32_t*)(framebuffer + (y * bootInfo->vbe_pitch) + (x * (bootInfo->vbe_bpp / 8)));

    // Write the color to the calculated address.
    *pixel_ptr = color;
}