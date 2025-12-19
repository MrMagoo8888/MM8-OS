// color.c

#include "color.h" // Include your header
#include "string.h"  // For strcmp, strtoul, etc.
#include "stdio.h" 
#include <arch/i686/screen_defs.h>
#include "ctype.h"
#include "stddef.h"

// Helper to parse a 1 or 2 digit hex string to a byte
static unsigned char hex_to_byte(const char* hex_str) {
    unsigned char val = 0;
    while (*hex_str) {
        char c = *hex_str++;
        if (c >= '0' && c <= '9')
            val = (val << 4) + (c - '0');
        else if (c >= 'a' && c <= 'f')
            val = (val << 4) + (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            val = (val << 4) + (c - 'A' + 10);
    }
    return val;
}

// Lookup table for named colors to avoid long if-else chains
typedef struct {
    const char* name;
    unsigned char code;
} ColorMap;

static const ColorMap colors[] = {
    {"blue", Color_Blue},
    {"black", Color_Black},
    {"green", Color_Green},
    {"cyan", Color_Cyan},
    {"red", Color_Red},
    {"magenta", Color_Magenta},
    {"brown", Color_Brown},
    {"lightgray", Color_LightGray},
    {"darkgray", Color_DarkGray},
    {"lightblue", Color_LightBlue},
    {"lightgreen", Color_LightGreen},
    {"lightcyan", Color_LightCyan},
    {"lightred", Color_LightRed},
    {"lightmagenta", Color_LightMagenta},
    {"yellow", Color_Yellow},
    {"white", Color_White},
    {NULL, 0}
};


// Global variable definition (if not defined in screendefs.c)
// unsigned char default_screen_color = COLOR_GREEN; 

void handle_color(const char* input) {
    // The argument starts right after "color " (6 characters in)
    const char* color_arg = input + 6;
    unsigned char new_color_code = 0;
    int found = 0;
    
    // --- 1. Try to match named colors ---
    for (int i = 0; colors[i].name != NULL; i++) {
        if (strcmp(color_arg, colors[i].name) == 0) {
            new_color_code = colors[i].code;
            found = 1;
            break;
        }
    }
    
    // --- 2. Try to parse as a number (e.g., 0x0A, 10) ---
    if (!found && color_arg[0] != '\0') {
        new_color_code = hex_to_byte(color_arg);
    } else if (!found) {
        // No argument provided (e.g., user just typed "color")
        printf("Usage: color <name | hex_code>\n");
        printf("Example: color yellow\n");
        printf("Example: color 0E\n");
        return;
    }
    
    // --- 3. Update the variable ---
    DEFAULT_COLOR = new_color_code;
    
    // --- 4. Refresh the screen with the new color ---
    refresh_screen_color();

    // --- 5. Print confirmation message in the new color ---
    printf("Color set to 0x%X\n", (unsigned int)new_color_code);
}