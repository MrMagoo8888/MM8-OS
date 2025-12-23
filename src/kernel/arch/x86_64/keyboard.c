#include "stdio.h"
#include "irq.h"
#include "io.h"
#include <stdint.h>
#include "keyboard.h"
#include "stdbool.h"
#include "memory.h"
#include "string.h"
#include "pic.h"
#include "stddef.h"

#include "screen_defs.h"

typedef enum {
    INPUT_MODE_NONE,
    INPUT_MODE_GETS,  // Waiting for gets()
    INPUT_MODE_GETCH, // Waiting for getch()
} InputMode;
static volatile InputMode g_CurrentInputMode = INPUT_MODE_NONE;

// --- Static variables for buffered input ---
#define INPUT_BUFFER_SIZE 256
static char g_InputBuffer[INPUT_BUFFER_SIZE];
static int g_InputBufferIndex = 0;
static volatile bool g_InputLineReady = false;

// --- Static variables for single character input (getch) ---
static volatile int g_CharBuffer = -1; // -1 means empty
static volatile bool g_CharReady = false;

static bool g_ShiftPressed = false;
static bool g_CtrlPressed = false;
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
    for (int i = g_ScreenX; i < SCREEN_WIDTH; i++) {
        putc(' ');
    }
    setcursor(prompt_len + g_InputBufferIndex, g_ScreenY);
}

void keyboard_irq_handler(Registers* regs) {
    uint8_t scancode = i686_inb(KEYBOARD_DATA_PORT);

    // Handle shift press/release
    if (scancode == 0x2A || scancode == 0x36) { // Left or Right Shift pressed
        g_ShiftPressed = true;
        i686_PIC_SendEndOfInterrupt(1);
        return; // Return after sending EOI
    } else if (scancode == 0xAA || scancode == 0xB6) { // Left or Right Shift released
        g_ShiftPressed = false;
        i686_PIC_SendEndOfInterrupt(1);
        return; // Return after sending EOI
    } else if (scancode == 0x1D) { // Ctrl pressed
        g_CtrlPressed = true;
        i686_PIC_SendEndOfInterrupt(1);
        return; // Return after sending EOI
    } else if (scancode == 0x9D) { // Ctrl released
        g_CtrlPressed = false;
        i686_PIC_SendEndOfInterrupt(1);
        return; // Return after sending EOI
    } else if (scancode == 0xE0) {
        extended = 1;
        i686_PIC_SendEndOfInterrupt(1);
        return; // Return after sending EOI
    }

    if (extended) {
        switch (scancode) {
            case 0x38: // AltGr pressed
                g_AltGrPressed = true;
                break;
            case 0xB8: // AltGr released
                g_AltGrPressed = false;
                break;
            case 0x48: // Up Arrow
                g_CharBuffer = KEY_UP; g_CharReady = true;
                break;
            case 0x50: // Down Arrow
                g_CharBuffer = KEY_DOWN; g_CharReady = true;
                break;
            case 0x4B: // Left Arrow
                g_CharBuffer = KEY_LEFT; g_CharReady = true;
                break;
            case 0x4D: // Right Arrow
                g_CharBuffer = KEY_RIGHT; g_CharReady = true;
                break;
            case 0x49: { // PageUp (now for command history)
                // For now, we'll let the editor handle this.
                // In the future, this could be used for scrolling.
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
            case 0x53: // Delete
                g_CharBuffer = KEY_DELETE; g_CharReady = true;
        }
        extended = 0;
        i686_PIC_SendEndOfInterrupt(1);
        return; // Return after sending EOI
    }

    // Ignore key release codes
    if (scancode & 0x80) {
        return;
    }


    char c;
    if (g_CtrlPressed)
        c = scancode_ascii[scancode] & 0x1F; // Create control character
    else if (g_AltGrPressed)
        c = scancode_ascii_altgr[scancode];
    else if (g_ShiftPressed)
        c = scancode_ascii_shifted[scancode];
    else
        c = scancode_ascii[scancode];

    if (g_CurrentInputMode == INPUT_MODE_GETCH) {
        if (c != 0 || g_CharBuffer != -1) { // Pass through printable chars or special keys
            if (!g_CharReady) {
                if (c != 0) g_CharBuffer = c; // Prioritize special keys set earlier
                g_CharReady = true;
            }
        }
    } else if (g_CurrentInputMode == INPUT_MODE_GETS) {
        // Handle special keys first
        if (g_CharBuffer == KEY_DELETE) {
            if (g_InputBufferIndex < strlen(g_InputBuffer)) {
                memmove(&g_InputBuffer[g_InputBufferIndex], &g_InputBuffer[g_InputBufferIndex + 1], strlen(g_InputBuffer) - g_InputBufferIndex);
                redraw_input_line();
            }
            g_CharBuffer = -1; // Consume the delete key
            g_CharReady = false; 
        }
        // Now handle printable characters
        else if (!g_InputLineReady) {
            if (c == '\n') {
                g_InputBuffer[g_InputBufferIndex] = '\0';
                g_InputLineReady = true;
                g_HistoryNavIndex = -1; // Reset history navigation on enter
                putc('\n');
            } else if (c == '\b') {
                if (g_InputBufferIndex > 0) {
                    g_InputBufferIndex--;
                    // Let putc handle backspace logic on screen
                    putc('\b');
                }
            } else if (c != 0) {
                if (g_InputBufferIndex < INPUT_BUFFER_SIZE - 1) {
                    g_InputBuffer[g_InputBufferIndex++] = c;
                    putc(c); // Echo character
                }
            }
        }
    }

    i686_PIC_SendEndOfInterrupt(1); // test**
}

void i686_Keyboard_Initialize(char (*history_buffer)[256], int* history_count, int* history_index, int history_size) {
    g_HistoryBuffer = history_buffer;
    g_HistoryCount = history_count;
    g_HistoryIndexPtr = history_index;
    g_HistorySize = history_size;
    i686_IRQ_RegisterHandler(1, keyboard_irq_handler);
}

void gets(char* buffer, int size) {
    g_CurrentInputMode = INPUT_MODE_GETS;

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

    g_CurrentInputMode = INPUT_MODE_NONE;

    __asm__ volatile("sti"); // Re-enable interrupts
}

int getch() {
    g_CurrentInputMode = INPUT_MODE_GETCH;

    while (!g_CharReady) {
        __asm__ volatile("hlt"); // Wait for a key press
    }

    __asm__ volatile("cli"); // Disable interrupts for critical section

    int c = g_CharBuffer;
    g_CharReady = false;
    g_CharBuffer = -1;

    g_CurrentInputMode = INPUT_MODE_NONE;

    __asm__ volatile("sti"); // Re-enable interrupts

    return c;
}