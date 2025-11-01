#pragma once

#include "isr.h"
#include "screen_defs.h"   // use shared definitions (SCROLLBACK_LINES, SCREEN_WIDTH, etc.)

// Special key codes for getch()
enum {
    KEY_UP = 256,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_DELETE,
};

void i686_Keyboard_Initialize(char (*history_buffer)[256], int* history_count, int* history_index, int history_size);
void keyboard_irq_handler(Registers* regs);

// Reads a line of input from the keyboard into the provided buffer.
void gets(char* buffer, int size);

// Reads a single character or special key code from the keyboard.
int getch();