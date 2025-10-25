#include "stdio.h"
#include "irq.h"
#include "io.h"
#include <stdint.h>
#include "keyboard.h"
#include "stdbool.h"
#include "memory.h"
#include "string.h"
#include "stddef.h"

#include "screen_defs.h"

// --- Static variables for buffered input ---
#define INPUT_BUFFER_SIZE 256
static char g_InputBuffer[INPUT_BUFFER_SIZE];
static int g_InputBufferIndex = 0;
static volatile bool g_InputLineReady = false;
static bool g_ShiftPressed = false;
static bool g_AltGrPressed = false;

// --- Command History ---
static char (*g_HistoryBuffer)[256] = NULL;
static int* g_HistoryCount = NULL;
static int* g_HistoryIndexPtr = NULL; // Pointer to main.c's g_HistoryIndex
static int g_HistorySize = 0;
static int g_HistoryNavIndex = -1; // How far back we are in history. -1 = not navigating. 0 = most recent.

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

static void redraw_input_line() {
    // Go to the start of the input area
    int prompt_len = 2; // "> "
    g_ScreenX = prompt_len;
    setcursor(g_ScreenX, g_ScreenY);

    // Print the current buffer
    printf("%s", g_InputBuffer);

    // Clear the rest of the line to erase old characters
    for (int i = strlen(g_InputBuffer); i < SCREEN_WIDTH - prompt_len; i++) {
        putc(' ');
    }
    setcursor(prompt_len + g_InputBufferIndex, g_ScreenY);
}

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
                if (g_ScreenX > 0) {
                    g_ScreenX--;
                    setcursor(g_ScreenX, g_ScreenY);
                }
                break;
            case 0x4D: // Right Arrow
                if (g_ScreenX < SCREEN_WIDTH - 1) {
                    g_ScreenX++;
                    setcursor(g_ScreenX, g_ScreenY);
                }
                break;
            case 0x48: // Up Arrow
                if (g_ScreenY > 0) {
                    g_ScreenY--;
                    setcursor(g_ScreenX, g_ScreenY);
                }
                break;
            case 0x50: // Down Arrow
                if (g_ScreenY < SCREEN_HEIGHT - 1) {
                    g_ScreenY++;
                    setcursor(g_ScreenX, g_ScreenY);
                }
                break;
            case 0x49: { // PageUp (now for command history)
                if (g_HistoryCount && *g_HistoryCount > 0) {
                    if (g_HistoryNavIndex < (*g_HistoryCount - 1)) {
                        g_HistoryNavIndex++;
                    }
                    // Correctly calculate the index in the circular buffer
                    int actual_index = (*g_HistoryIndexPtr - 1 - g_HistoryNavIndex + g_HistorySize) % g_HistorySize;
                    strcpy(g_InputBuffer, g_HistoryBuffer[actual_index]);
                    g_InputBufferIndex = strlen(g_InputBuffer);
                    redraw_input_line();
                }
                break;
            }
            case 0x51: { // PageDown (now for command history)
                if (g_HistoryNavIndex != -1) {
                    if (g_HistoryNavIndex > 0) {
                        g_HistoryNavIndex--;
                        // Correctly calculate the index in the circular buffer
                        int actual_index = (*g_HistoryIndexPtr - 1 - g_HistoryNavIndex + g_HistorySize) % g_HistorySize;
                        strcpy(g_InputBuffer, g_HistoryBuffer[actual_index]);
                    } else {
                        // Reached the end of history, clear the line
                        g_HistoryNavIndex = -1;
                        g_InputBuffer[0] = '\0';
                    }
                    g_InputBufferIndex = strlen(g_InputBuffer);
                    redraw_input_line();
                }
                break;
            }
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
        g_HistoryNavIndex = -1; // Reset history navigation on enter
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

void i686_Keyboard_Initialize(char (*history_buffer)[256], int* history_count, int* history_index, int history_size) {
    g_HistoryBuffer = history_buffer;
    g_HistoryCount = history_count;
    g_HistoryIndexPtr = history_index;
    g_HistorySize = history_size;
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
