#include "calc.h"
#include "stdio.h"
#include "graphics.h"
#include "string.h"
#include <arch/i686/keyboard.h> // For gets
#include "ctype.h" // For isdigit, to skip non-digit characters
#include "stdbool.h" // For bool type

// Simple atoi implementation (since strtod is a stub and we need integer parsing)
// This version does NOT skip leading whitespace; that's handled by skip_whitespace.
static int simple_atoi_internal(const char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    // Handle negative numbers
    if (str[0] == '-') {
        sign = -1;
        i++;
    } else if (str[0] == '+') {
        i++;
    }

    // Iterate through all digits and update the result
    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10 + (str[i] - '0');
        i++;
    }
    return sign * res;
}

// Helper to find the next non-whitespace character
static const char* skip_whitespace(const char* str) {
    while (*str == ' ') {
        str++;
    }
    return str;
}

static void perform_single_calculation(const char* expression, int y_pos) {
    const char* current_ptr = expression;
    char buffer[128];

    if (*current_ptr == '\0') { // Handle empty expression
        graphics_draw_string(20, y_pos, "Supported operators: +, -, *, /", 0xFFFFFFFF);
        return;
    }

    // Parse first number
    int num1 = simple_atoi_internal(current_ptr);
    // Advance current_ptr past the digits and sign of num1
    if (*current_ptr == '-' || *current_ptr == '+') current_ptr++;
    while (*current_ptr >= '0' && *current_ptr <= '9') {
        current_ptr++;
    }

    current_ptr = skip_whitespace(current_ptr); // Skip spaces after num1

    // Parse operator
    char op = *current_ptr;
    if (op == '\0') {
        graphics_draw_string(20, y_pos, "Error: Missing operator or second number.", 0xFFFF0000);
        return;
    }
    current_ptr++; // Advance past operator
    current_ptr = skip_whitespace(current_ptr); // Skip spaces after operator

    // Parse second number
    if (*current_ptr == '\0') {
        graphics_draw_string(20, y_pos, "Error: Missing second number.", 0xFFFF0000);
        return;
    }
    int num2 = simple_atoi_internal(current_ptr);

    int result;
    bool error = false;

    switch (op) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/':
            if (num2 == 0) {
                graphics_draw_string(20, y_pos, "Error: Division by zero.", 0xFFFF0000);
                error = true;
            } else {
                result = num1 / num2;
            }
            break;
        default:
            sprintf(buffer, "Error: Unsupported operator '%c'.", op);
            graphics_draw_string(20, y_pos, buffer, 0xFFFF0000);
            error = true;
            break;
    }

    if (!error) {
        sprintf(buffer, "Result: %d", result);
        graphics_draw_string(20, y_pos, buffer, 0xFFFFFFFF);
    }
}

static void run_calc_interactive_mode() {
    int y = 20;
    graphics_clear_screen(0xFF000055); // Dark blue background for calc
    graphics_draw_string(20, y, "MM8-OS Calculator", 0xFFFFFFFF); y+=10;
    graphics_draw_string(20, y, "Usage: <num1> <op> <num2>", 0xFFFFFFFF); y+=10;
    graphics_draw_string(20, y, "Type 'exit' to return.", 0xFFFFFFFF); y+=10;
    y+=10;

    int input_y = y;
    int result_y = y + 10;

    char input_buffer[256];
    while (true) {
        graphics_draw_rect(0, input_y, g_vbe_screen_info->width, 18, 0xFF000055); // Clear input/result lines
        graphics_draw_string(20, input_y, "calc> ", 0xFFFFFFFF);
        // This is a problem - gets() is blocking and doesn't work with our graphical model.
        // For now, we will just exit. A proper graphical input box is needed.
        // gets(input_buffer, sizeof(input_buffer));
        graphics_draw_string(20, result_y, "Interactive mode not fully supported in graphics yet. Exiting.", 0xFFFFFF00);
        break;

        // Convert input to lowercase for command comparison
        char lower_input[256];
        strcpy(lower_input, input_buffer);
        for (int i = 0; lower_input[i]; i++) {
            lower_input[i] = tolower(lower_input[i]);
        }

        if (strcmp(lower_input, "exit") == 0) {
            break;
        }

        if (input_buffer[0] == '\0') {
            continue; // Ignore empty input
        }

        perform_single_calculation(input_buffer, result_y);
    }
    // Return to main console
    graphics_clear_screen(0xFFFF5486);
}

void handle_calc(const char* input) {
    const char* args = input + strlen("calc"); // Skip "calc" part of the command
    args = skip_whitespace(args);

    if (*args == '\0') {
        run_calc_interactive_mode();
    } else {
        perform_single_calculation(args, 120);
    }
}