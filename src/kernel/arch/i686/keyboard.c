#include "irq.h"
#include "io.h"
#include <stdio.h>
#include <stdint.h>

extern int g_ScreenX, g_ScreenY;
extern int SCREEN_WIDTH, SCREEN_HEIGHT;

#define KEYBOARD_DATA_PORT 0x60

static int extended = 0; // Track extended key prefix

static const char scancode_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
    // ... fill in as needed ...
};

void keyboard_irq_handler(Registers* regs) {
    uint8_t scancode = i686_inb(KEYBOARD_DATA_PORT);

    // Extended key prefix
    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    // Handle extended arrow keys
    if (extended) {
        switch (scancode) {
            case 0x4B: // Left Arrow
                if (g_ScreenX > 0) g_ScreenX--;
                setcursor(g_ScreenX, g_ScreenY);
                break;
            case 0x4D: // Right Arrow
                if (g_ScreenX < SCREEN_WIDTH - 1) g_ScreenX++;
                setcursor(g_ScreenX, g_ScreenY);
                break;
            case 0x48: // Up Arrow
                if (g_ScreenY > 0) g_ScreenY--;
                setcursor(g_ScreenX, g_ScreenY);
                break;
            case 0x50: // Down Arrow
                if (g_ScreenY < SCREEN_HEIGHT - 1) g_ScreenY++;
                setcursor(g_ScreenX, g_ScreenY);
                break;
            // You can add Home/End/PageUp/PageDown here too
        }
        extended = 0;
        return;
    }

    // Reset extended flag if not used
    extended = 0;

    // Ignore release codes (scancode >= 0x80)
    if (scancode < 128) {
        char c = scancode_ascii[scancode];
        if (c == '\b') { // Backspace
            if (g_ScreenX > 0) {
                g_ScreenX--;
                putchr(g_ScreenX, g_ScreenY, ' ');
                setcursor(g_ScreenX, g_ScreenY);
            }
        } else if (c) {
            putc(c);
        }
    }
}

void i686_Keyboard_Initialize() {
    i686_IRQ_RegisterHandler(1, keyboard_irq_handler); // <-- FIXED: use IRQ registration
}