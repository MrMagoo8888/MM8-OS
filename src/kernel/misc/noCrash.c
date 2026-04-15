#include "noCrash.h"
#include "stdio.h"
#include "graphics.h"
#include "time.h"
#include "vbe.h"

void init_tests() {
    // No longer blocks initialization
}

void pixelLoop() {
    static uint32_t last_tick = 0;
    if (!g_vbe_screen) return;

    // Check if 50ms has passed since the last successful run
    if (time_is_periodic(&last_tick, 50))
    {
        // Use coordinates relative to the screen edge (bottom right)
        int base_x = g_vbe_screen->width - 20;
        int base_y = g_vbe_screen->height - 20;

        // Simple toggle animation based on current uptime
        uint32_t phase = (get_uptime_ms() / 100) % 4;

        // Use RGB Hex literals instead of VGA indices
        // 0xFFFFFF = White, 0x555555 = Dark Gray
        uint32_t active_color = 0xFFFFFF;
        uint32_t idle_color   = 0x555555;

        draw_pixel(base_x,     base_y,     (phase == 0) ? active_color : idle_color);
        draw_pixel(base_x + 5, base_y,     (phase == 1) ? active_color : idle_color);
        draw_pixel(base_x + 5, base_y + 5, (phase == 2) ? active_color : idle_color);
        draw_pixel(base_x,     base_y + 5, (phase == 3) ? active_color : idle_color);
    }
}