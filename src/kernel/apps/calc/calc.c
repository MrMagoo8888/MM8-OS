#include "calc.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h" // For gets
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

static void perform_single_calculation(const char* expression) {
    const char* current_ptr = expression;

    if (*current_ptr == '\0') { // Handle empty expression
        printf("Supported operators: +, -, *, /\n");
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
        printf("Error: Missing operator or second number.\n");
        return;
    }
    current_ptr++; // Advance past operator
    current_ptr = skip_whitespace(current_ptr); // Skip spaces after operator

    // Parse second number
    if (*current_ptr == '\0') {
        printf("Error: Missing second number.\n");
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
                printf("Error: Division by zero.\n");
                error = true;
            } else {
                result = num1 / num2;
            }
            break;
        default: printf("Error: Unsupported operator '%c'. Supported: +, -, *, /\n", op); error = true; break;
    }

    if (!error) {
        printf("Result: %d\n", result);
    }
}

static void run_calc_interactive_mode() {
    clrscr();
    printf("MM8-OS Calculator\n");
    printf("Usage: <number1> <operator> <number2>\n");
    printf("Supported operators: +, -, *, /\n");
    printf("Type 'exit' to return to the main shell.\n");
    printf("Type 'clear' or 'cls' to clear the screen.\n\n");

    char input_buffer[256];
    while (true) {
        printf("calc> ");
        gets(input_buffer, sizeof(input_buffer));

        // Convert input to lowercase for command comparison
        char lower_input[256];
        strcpy(lower_input, input_buffer);
        for (int i = 0; lower_input[i]; i++) {
            lower_input[i] = tolower(lower_input[i]);
        }

        if (strcmp(lower_input, "exit") == 0) {
            printf("Exiting calculator.\n");
            break;
        }
        if (strcmp(lower_input, "clear") == 0 || strcmp(lower_input, "cls") == 0) {
            clrscr();
            printf("MM8-OS Calculator\n");
            printf("Usage: <number1> <operator> <number2>\n");
            printf("Supported operators: +, -, *, /\n");
            printf("Type 'exit' to return to the main shell.\n");
            printf("Type 'clear' or 'cls' to clear the screen.\n\n");
            continue;
        }
        if (input_buffer[0] == '\0') {
            continue; // Ignore empty input
        }

        perform_single_calculation(input_buffer);
    }
    clrscr(); // Clear screen after exiting calc mode to return to a clean main shell
}

void handle_calc(const char* input) {
    const char* args = input + strlen("calc"); // Skip "calc" part of the command
    args = skip_whitespace(args);

    if (*args == '\0') {
        run_calc_interactive_mode();
    } else {
        perform_single_calculation(args);
    }
}