#include "stdio.h"
#include "irq.h"
#include "io.h"
#include <stdint.h>
#include "keyboard.h"
#include "stdbool.h"
#include "memory.h"
#include "string.h"

#include "screen_defs.h"

// --- Static variables for buffered input ---
#define INPUT_BUFFER_SIZE 256
static char g_InputBuffer[INPUT_BUFFER_SIZE];
static int g_InputBufferIndex = 0;
static volatile bool g_InputLineReady = false;
static bool g_ShiftPressed = false;
static bool g_AltGrPressed = false;

extern uint8_t* g_ScreenBuffer;
extern int g_ScreenX, g_ScreenY;

#define KEYBOARD_DATA_PORT 0x60

static int extended = 0; // Track extended key prefix

// Scancode to ASCII mapping
static const char scancode_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // 0-14
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, // 15-29
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '#', // 30-43
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, // 44-58
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 59-85
    '\\', // 86 (scancode 0x56)
};
// Scancode to ASCII mapping (shifted)
static const char scancode_ascii_shifted[128] = {
    0,  27, '!', '"', '\x9C', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', // 0-14, \x9C is £
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, // 15-29
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '@', '\xAA', 0, '~', // 30-43, \xAA is ¬
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, // 44-58
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 59-85
    '|', // 86 (scancode 0x56)
};
// Scancode to ASCII mapping (AltGr)
// Using CP437 character codes for accented letters
static const char scancode_ascii_altgr[128] = {
    0, 0, 0, 0, 0 /* € is not in CP437 */, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '\x82'/*é*/, 0, 0, 0, '\x97'/*ú*/, '\xA1'/*í*/, '\xA2'/*ó*/, 0, 0, 0, 0, 0,
    '\xA0'/*á*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // ... rest are 0
};

void keyboard_irq_handler(Registers* regs) {
    uint8_t scancode = i686_inb(KEYBOARD_DATA_PORT);

    // Handle shift press/release
    if (scancode == 0x2A || scancode == 0x36) { // Left or Right Shift pressed
        g_ShiftPressed = true;
        return;
    } else if (scancode == 0xAA || scancode == 0xB6) { // Left or Right Shift released
        g_ShiftPressed = false;
        return;
    } else if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    if (extended) {
        switch (scancode) {
            case 0x38: // AltGr pressed
                g_AltGrPressed = true;
                break;
            case 0xB8: // AltGr released
                g_AltGrPressed = false;
                break;
            case 0x4B: // Left Arrow
                // For simplicity, we'll leave this and other navigation for a future step
                break;
            case 0x4D: // Right Arrow
                break;
            case 0x48: // Up Arrow
                break;
            case 0x50: // Down Arrow
                break;
            case 0x49: // PageUp
                view_scrollback_up();
                break;
            case 0x51: // PageDown
                view_scrollback_down();
                break;
            case 0x53: // Delete - Not implemented in this simplified version
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

    char c;
    if (g_AltGrPressed)
        c = scancode_ascii_altgr[scancode];
    else if (g_ShiftPressed)
        c = scancode_ascii_shifted[scancode];
    else
        c = scancode_ascii[scancode];

    if (c == '\n') {
        g_InputBuffer[g_InputBufferIndex] = '\0';
        g_InputLineReady = true;
        putc('\n');
    } else if (c == '\b') {
        if (g_InputBufferIndex > 0) {
            g_ScreenX--;
            g_InputBufferIndex--;
            putchr(g_ScreenX, g_ScreenY, ' ');
            setcursor(g_ScreenX, g_ScreenY);
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
    memset(g_InputBuffer, 0, sizeof(g_InputBuffer)); // CRITICAL: Clear buffer for next use
    g_InputLineReady = false;

    __asm__ volatile("sti"); // Re-enable interrupts
}
