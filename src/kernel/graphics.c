#include "graphics.h"
#include "stdio.h" // For putchr, putcolor
#include "arch/i686/screen_defs.h" // For SCREEN_WIDTH, SCREEN_HEIGHT

void draw_pixel(int x, int y, char c, uint8_t color) {
    // Bounds checking to prevent writing outside the screen buffer
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        putchr(x, y, c);
        putcolor(x, y, color);
    }
}

void draw_rect(int x, int y, int width, int height, char c, uint8_t color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            draw_pixel(x + i, y + j, c, color);
        }
    }
}

void draw_circle(int centerX, int centerY, int radius, char c, uint8_t color) {
    // Using a simple square-based check for a filled circle
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            // Check if the point (x, y) is inside the circle using the formula x^2 + y^2 <= r^2
            if (x * x + y * y <= radius * radius) {
                draw_pixel(centerX + x, centerY + y, c, color);
            }
        }
    }
}