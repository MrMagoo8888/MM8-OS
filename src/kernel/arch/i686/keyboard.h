#pragma once

#include "isr.h"

// Ensure SCROLLBACK_LINES and SCREEN_WIDTH are defined as macros before this line
#ifndef SCROLLBACK_LINES
#define SCROLLBACK_LINES 2 // or another suitable constant value
#endif

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 80 // or another suitable constant value
#endif

extern char scrollback_buffer[SCROLLBACK_LINES][SCREEN_WIDTH];
extern int scrollback_start;
extern int scrollback_count;
extern int scrollback_view;

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