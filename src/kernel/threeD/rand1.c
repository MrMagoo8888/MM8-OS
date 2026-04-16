#include "rand1.h"
#include "stdio.h"
#include "graphics.h"
#include "vbe.h"
#include "memory.h"
#include "stdlib.h"
#include "time.h"
#include "arch/i686/keyboard.h"

void handle_rand1() {
    rand1_test();
}

void handle_rand2() {
    rand2_test();
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
    // A task to make a random pixel display based on a random seed and holds the screen until a key is pressed.
    printf("Random Pixel Test: Press any key to exit.\n");

    clrscr();

    graphics_set_double_buffering(true);

    if (!g_DoubleBufferEnabled) {
        printf("Error: Double buffering not available.\n");
        getch();
        return;
    }

    graphics_clear_buffer(0x00000000);
    

    srand(get_uptime_ms());

    
    while (!kbhit()) {
        int x = rand() % g_vbe_screen->width;
        int y = rand() % g_vbe_screen->height;
        uint32_t color = rand() % 0xFFFFFF;
        draw_pixel(x, y, color);
    }

    getch(); // Consume the keypress that exited the loop
}