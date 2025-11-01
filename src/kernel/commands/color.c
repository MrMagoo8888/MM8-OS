// color.c

#include "color.h" // Include your header
#include "string.h"  // For strcmp, strtoul, etc.
#include "stdio.h" 
#include <arch/i686/screen_defs.h>
#include "ctype.h"

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


// Global variable definition (if not defined in screendefs.c)
// unsigned char default_screen_color = COLOR_GREEN; 

void handle_color(const char* input) {
    // The argument starts right after "color " (6 characters in)
    const char* color_arg = input + 6;
    unsigned char new_color_code = 0;
    
    // --- 1. Try to match named colors ---
    if (strcmp(color_arg, "blue") == 0) {
        new_color_code = Color_Blue;
    } else if (strcmp(color_arg, "black") == 0) {
        new_color_code = Color_Black;
    } else if (strcmp(color_arg, "green") == 0) {
        new_color_code = Color_Green;
    } else if (strcmp(color_arg, "cyan") == 0) {
        new_color_code = Color_Cyan;
    } else if (strcmp(color_arg, "red") == 0) {
        new_color_code = Color_Red;
    } else if (strcmp(color_arg, "magenta") == 0) {
        new_color_code = Color_Magenta;
    } else if (strcmp(color_arg, "brown") == 0) {
        new_color_code = Color_Brown;
    } else if (strcmp(color_arg, "lightgray") == 0) {
        new_color_code = Color_LightGray;
    } else if (strcmp(color_arg, "darkgray") == 0) {
        new_color_code = Color_DarkGray;
    } else if (strcmp(color_arg, "lightblue") == 0) {
        new_color_code = Color_LightBlue;
    } else if (strcmp(color_arg, "lightgreen") == 0) {
        new_color_code = Color_LightGreen;
    } else if (strcmp(color_arg, "lightcyan") == 0) {
        new_color_code = Color_LightCyan;
    } else if (strcmp(color_arg, "lightred") == 0) {
        new_color_code = Color_LightRed;
    } else if (strcmp(color_arg, "lightmagenta") == 0) {
        new_color_code = Color_LightMagenta;
    } else if (strcmp(color_arg, "yellow") == 0) {
        new_color_code = Color_Yellow;
    } else if (strcmp(color_arg, "white") == 0) {
        new_color_code = Color_White;
    } 
    // ... add more named colors (green, red, white, etc.) ...
    
    // --- 2. Try to parse as a number (e.g., 0x0A, 10) ---
    else if (color_arg[0] != '\0') {
        new_color_code = hex_to_byte(color_arg);
    } else {
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