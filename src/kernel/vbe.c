#include "vbe.h"
#include "memory.h"

// Global variable to hold the VBE info passed from the bootloader.
static VBE_INFO g_vbe_info;

void VBE_Initialize(VBE_INFO* vbe_info_from_bootloader)
{
    // Copy the VBE info passed by the bootloader into our kernel's global structure.
    memcpy(&g_vbe_info, vbe_info_from_bootloader, sizeof(VBE_INFO));
}

void VBE_draw_pixel(int x, int y, uint32_t color)
{
    // Ensure we don't draw outside the screen boundaries.
    if (x < 0 || y < 0 || x >= g_vbe_info.width || y >= g_vbe_info.height) {
        return;
    }

    // Calculate the linear address of the pixel.
    // The framebuffer is a 1D array of pixels.
    // The address is: (y * pitch) + (x * bytes_per_pixel)
    uint8_t* framebuffer = (uint8_t*)g_vbe_info.vbe_physical_buffer;
    uint32_t* pixel_ptr = (uint32_t*)(framebuffer + (y * g_vbe_info.vbe_pitch) + (x * (g_vbe_info.bpp / 8)));

    // Write the color to the calculated address.
    *pixel_ptr = color;
}