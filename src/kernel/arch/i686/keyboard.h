#pragma once

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

void i686_Keyboard_Initialize();
void keyboard_irq_handler(Registers* regs);

// Reads a line of input from the keyboard into the provided buffer.
void gets(char* buffer, int size);