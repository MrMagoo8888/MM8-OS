// color.c

#include "color.h" // Include your header
#include "string.h"  // For strcmp, strtoul, etc.
#include "stdio.h" 
#include <arch/i686/screen_defs.h>
// #include "screendefs.h" // Include your screen definitions file for color constants



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
        // Use a function like strtoul or your custom string-to-number function
        // This is a common pattern in OS dev, but you may need to implement strtoul.
        // For simplicity, here's a conceptual call:
        // new_color_code = (unsigned char)your_str_to_int_func(color_arg);
        
        // **If using simple numerical codes (0-15):**
        // A simple approach is to check if the first character is a digit and convert it.
        if (color_arg[0] >= '0' && color_arg[0] <= '9') {
            new_color_code = color_arg[0] - '0'; // For single-digit decimal codes
        } else {
             printf("Error: Invalid color name or code.\n");
             return;
        }
    } else {
        // No argument provided (e.g., user just typed "color")
        printf("Usage: color <name or code>\n");
        return;
    }
    
    // --- 3. Update the variable ---
    default_screen_color = new_color_code;
    printf("Default color set to code: 0x%X\n", (unsigned int)new_color_code);
    
    // You may also want to call a function here to immediately apply the change
    // to the entire screen/console if needed.
    // update_screen_colors();
}