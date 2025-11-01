#include <arch/i686/screen_defs.h>
// color.h

#ifndef COLOR_H
#define COLOR_H

// Function prototype for the command handler
void handle_color(const char* input);

// Declare the external default color variable (for consistency, 
// you can also keep this in screendefs.h as planned)
extern unsigned char default_screen_color;

#endif // COLOR_H