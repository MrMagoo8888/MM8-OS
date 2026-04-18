#include "rand1.h"
#include "stdio.h"
#include "graphics.h"
#include "vbe.h"
#include "memory.h"
#include "stdlib.h"
#include "time.h"
#include "font.h"
#include "arch/i686/keyboard.h"
#include "string.h"

void handle_rand1() {
    rand1_test();
}

void handle_rand2() {
    rand2_test();
}

void handle_rand3() {
    rand3_test();
}

void handle_rand4() {
    rand4_test();
}

// Helper to draw a single character to the VBE framebuffer
static void draw_char_vbe(int x, int y, char c, uint32_t color) {
    if (c < 32 || c > 127) return;
    const uint8_t* glyph = font8x8_basic[c - 32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if ((glyph[row] >> (7 - col)) & 1) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

// Helper to draw a string to the VBE framebuffer
static void draw_string_vbe(int x, int y, const char* str, uint32_t color) {
    while (*str) {
        draw_char_vbe(x, y, *str, color);
        x += 8;
        str++;
    }
}

// Helper to draw a character scaled by 1.4x (~11x11 pixels)
static void draw_char_vbe_large(int x, int y, char c, uint32_t color) {
    if (c < 32 || c > 127) return;
    const uint8_t* glyph = font8x8_basic[c - 32];
    // 8 * 1.4 = 11.2, we use 11
    for (int row = 0; row < 11; row++) {
        for (int col = 0; col < 11; col++) {
            // Map 11x11 coordinate back to 8x8 bitmap
            int src_x = (col * 8) / 11;
            int src_y = (row * 8) / 11;
            if ((glyph[src_y] >> (7 - src_x)) & 1) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

static void draw_string_vbe_large(int x, int y, const char* str, uint32_t color) {
    while (*str) {
        draw_char_vbe_large(x, y, *str, color);
        // Offset by 11 pixels plus a 1-pixel gap
        x += 12;
        str++;
    }
}

void rand1_test() {
    // A task to make a random pixel display based on a random seed and holds the screen until a key is pressed.
    printf("Random Pixel Test: Press any key to exit.\n");

    srand(get_uptime_ms());

    while (!kbhit()) {
        int x = rand() % g_vbe_screen->width;
        int y = rand() % g_vbe_screen->height;
        uint32_t color = rand() % 0xFFFFFF;
        draw_pixel(x, y, color);
    }

    getch(); // Consume the keypress that exited the loop
}

void rand2_test() {
    // Fully black background version with double buffering
    printf("Random Pixel Test 2 (Black Background): Press any key to exit.\n");
    clrscr();
    graphics_set_double_buffering(true);
    if (!g_DoubleBufferEnabled) {
        printf("Error: Double buffering not available.\n");
        getch();
        return;
    }

    // Set screen fully black initially
    graphics_clear_buffer(0x00000000); 
    srand(get_uptime_ms());

    while (!kbhit()) {
        // Batch drawing for performance before swapping
        for (int i = 0; i < 100; i++) {
            int x = rand() % g_vbe_screen->width;
            int y = rand() % g_vbe_screen->height;
            uint32_t color = rand() % 0xFFFFFF;
            draw_pixel(x, y, color);
        }
        graphics_swap_buffer();
    }

    getch(); // Consume the keypress that exited the loop
    graphics_set_double_buffering(false);
    clrscr();
}

void rand3_test() {
    // Random pixels with a 3D "MM8-OS" logo overlay
    printf("Random Pixel Test 3 (3D Logo): Press any key to exit.\n");
    clrscr();
    graphics_set_double_buffering(true);

    if (!g_DoubleBufferEnabled) {
        printf("Error: Double buffering not available.\n");
        getch();
        return;
    }

    graphics_clear_buffer(0x00000000);
    srand(get_uptime_ms());

    const char* logo = "MM8-OS";
    int logo_x = (g_vbe_screen->width / 2) - (strlen(logo) * 4);
    int logo_y = (g_vbe_screen->height / 2) - 4;

    while (!kbhit()) {
        // 1. Draw random pixels
        for (int i = 0; i < 100; i++) {
            int x = rand() % g_vbe_screen->width;
            int y = rand() % g_vbe_screen->height;
            uint32_t color = rand() % 0xFFFFFF;
            draw_pixel(x, y, color);
        }

        // 2. Draw 3D Logo (Shadow then Front)
        // Shadow
        draw_string_vbe(logo_x + 2, logo_y + 2, logo, 0x00333333);
        // Front
        draw_string_vbe(logo_x, logo_y, logo, 0x00FFFFFF);

        // 3. Swap to screen
        graphics_swap_buffer();
    }

    getch(); 
    graphics_set_double_buffering(false);
    clrscr();
}

typedef struct {
    int x, y;
    int age;
    int max_age;
    uint32_t base_color;
} PixelLife;

void rand4_test() {
    printf("Random Pixel Test 4 (Life Cycle + Large Logo): Press any key to exit.\n");
    clrscr();
    graphics_set_double_buffering(true);

    if (!g_DoubleBufferEnabled) {
        printf("Error: Double buffering not available.\n");
        getch();
        return;
    }

    #define MAX_PARTICLES 1000 //250 old, 1000 looks kewl but may be slow on some machines
    PixelLife* particles = (PixelLife*)malloc(sizeof(PixelLife) * MAX_PARTICLES);
    if (!particles) {
        printf("Error: Failed to allocate particles memory.\n");
        getch();
        return;
    }

    memset(particles, 0, sizeof(PixelLife) * MAX_PARTICLES);
    srand(get_uptime_ms());

    // Initialize particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].max_age = 0; // Force respawn
    }

    const char* logo = "MM8-OS";
    // Center large logo (12 pixels per char width)
    int logo_x = (g_vbe_screen->width / 2) - ((strlen(logo) * 12) / 2);
    int logo_y = (g_vbe_screen->height / 2) - 6;

    while (!kbhit()) {
        graphics_clear_buffer(0x00000000);

        for (int i = 0; i < MAX_PARTICLES; i++) {
            // Respawn "dead" pixels
            if (particles[i].age >= particles[i].max_age) {
                particles[i].x = rand() % (g_vbe_screen->width - 2);
                particles[i].y = rand() % (g_vbe_screen->height - 2);
                particles[i].age = 0;
                particles[i].max_age = 20 + (rand() % 40);
                particles[i].base_color = rand() % 0xFFFFFF;
            }

            // Calculate brightness based on age (fade in then out)
            int half_life = particles[i].max_age / 2;
            int intensity = (particles[i].age < half_life) 
                ? (particles[i].age * 255 / half_life) 
                : (255 - ((particles[i].age - half_life) * 255 / half_life));
            
            uint32_t r = ((particles[i].base_color >> 16) & 0xFF) * intensity / 255;
            uint32_t g = ((particles[i].base_color >> 8) & 0xFF) * intensity / 255;
            uint32_t b = (particles[i].base_color & 0xFF) * intensity / 255;
            uint32_t final_color = (r << 16) | (g << 8) | b;

            // "Grow" the pixel: draw 2x2 if it's in the peak of its life
            draw_pixel(particles[i].x, particles[i].y, final_color);
            if (intensity > 150) {
                draw_pixel(particles[i].x + 1, particles[i].y, final_color);
                draw_pixel(particles[i].x, particles[i].y + 1, final_color);
                draw_pixel(particles[i].x + 1, particles[i].y + 1, final_color);
            }

            particles[i].age++;
        }

        // Draw 3D Scaled Logo
        draw_string_vbe_large(logo_x + 3, logo_y + 3, logo, 0x00222222); // Deep Shadow
        draw_string_vbe_large(logo_x, logo_y, logo, 0x00FFFFFF);        // White Front

        graphics_swap_buffer();
        
        // Small delay to make the lifecycle visible
        for(volatile int d = 0; d < 100000; d++);
    }

    getch();
    free(particles);
    graphics_set_double_buffering(false);
    clrscr();
}