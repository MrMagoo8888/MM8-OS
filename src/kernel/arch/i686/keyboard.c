#include "stdio.h"
#include "irq.h"
#include "io.h"
#include <stdint.h>
#include "keyboard.h"

#include "screen_defs.h"


extern uint8_t* g_ScreenBuffer;
extern int g_ScreenX, g_ScreenY;


void scrollback(int lines);
extern void scrollforward(int lines);

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
                scrollback_view += SCREEN_HEIGHT - 1;
                if (scrollback_view > scrollback_count)
                    scrollback_view = scrollback_count;
                // Show previous lines
                int idx = (scrollback_start + scrollback_count - scrollback_view) % SCROLLBACK_LINES;
                for (int y = 0; y < SCREEN_HEIGHT; y++)
                    for (int x = 0; x < SCREEN_WIDTH; x++)
                        putchr(x, y, scrollback_buffer[(idx + y) % SCROLLBACK_LINES][x]);
                setcursor(g_ScreenX, g_ScreenY);
                break;
            case 0x51: // PageDown
                scrollforward(SCREEN_HEIGHT - 1);
                break;
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

void scrollback(int lines)
{
    for (int i = 0; i < lines; i++) {
        // Save the top line to scrollback buffer
        memcpy(scrollback_buffer[(scrollback_start + scrollback_count) % SCROLLBACK_LINES],
               &g_ScreenBuffer[0], SCREEN_WIDTH);

        if (scrollback_count < SCROLLBACK_LINES)
            scrollback_count++;
        else
            scrollback_start = (scrollback_start + 1) % SCROLLBACK_LINES;
    }

    // Scroll the screen content up
    for (int y = lines; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }

    // Clear the bottom lines
    for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }

    g_ScreenY -= lines;
}

void scrollforward(int lines)
{
    if (scrollback_view == 0) return; // Already at live view

    int lines_to_restore = (scrollback_view < lines) ? scrollback_view : lines;
    scrollback_view -= lines_to_restore;

    // Restore lines from scrollback buffer
    int idx = (scrollback_start + scrollback_count - scrollback_view - SCREEN_HEIGHT) % SCROLLBACK_LINES;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y, scrollback_buffer[(idx + y) % SCROLLBACK_LINES][x]);
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
    setcursor(g_ScreenX, g_ScreenY);
}