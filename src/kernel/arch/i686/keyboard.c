#include "stdio.h"
#include "irq.h"
#include "io.h"
#include <stdint.h>
#include "keyboard.h"
#include "stdbool.h"
#include "memory.h"

#include "screen_defs.h"

// --- Static variables for buffered input ---
#define INPUT_BUFFER_SIZE 256
static char g_InputBuffer[INPUT_BUFFER_SIZE];
static int g_InputBufferIndex = 0;
static volatile bool g_InputLineReady = false;

extern uint8_t* g_ScreenBuffer;
extern int g_ScreenX, g_ScreenY;

#define KEYBOARD_DATA_PORT 0x60

static int extended = 0; // Track extended key prefix

// Scancode to ASCII mapping
static const char scancode_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
};

void keyboard_irq_handler(Registers* regs) {
    uint8_t scancode = i686_inb(KEYBOARD_DATA_PORT);

    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

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
            case 0x49: // PageUp
                view_scrollback_up();
                break;
            case 0x51: // PageDown
                view_scrollback_down();
                break;
        }
        extended = 0;
        return;
    }

    // Ignore key release codes
    if (scancode & 0x80) {
        return;
    }

    // --- Buffered Input Logic ---
    if (g_InputLineReady) {
        return; // Line buffer is full, waiting for gets() to read it
    }

    char c = scancode_ascii[scancode];

    if (c == '\n') {
        g_InputBuffer[g_InputBufferIndex] = '\0';
        g_InputLineReady = true;
        putc('\n');
    } else if (c == '\b') {
        if (g_InputBufferIndex > 0) {
            g_InputBufferIndex--;
            putc('\b');
        }
    } else if (c != 0) {
        if (g_InputBufferIndex < INPUT_BUFFER_SIZE - 1) {
            g_InputBuffer[g_InputBufferIndex++] = c;
            putc(c); // Echo character
        }
    }
}

void i686_Keyboard_Initialize() {
    i686_IRQ_RegisterHandler(1, keyboard_irq_handler);
}

void gets(char* buffer, int size) {
    while (!g_InputLineReady) {
        __asm__ volatile("hlt"); // Wait for an interrupt
    }

    __asm__ volatile("cli"); // Disable interrupts for critical section

    int i;
    for (i = 0; g_InputBuffer[i] != '\0' && i < size - 1; i++) {
        buffer[i] = g_InputBuffer[i];
    }
    buffer[i] = '\0';

    g_InputBufferIndex = 0;
    g_InputLineReady = false;

    __asm__ volatile("sti"); // Re-enable interrupts
}

