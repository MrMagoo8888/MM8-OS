#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/keyboard.h>
#include "string.h"

extern uint8_t __bss_start;
extern uint8_t __end;

void timer(Registers* regs)
{
    // This function is called by the system timer (IRQ 0).
    // For now, it does nothing but is required to acknowledge the interrupt.
}


// Command handler functions
void handle_help() {
    printf("Available commands:\n");
    printf(" - help: Show this message\n");
    printf(" - echo [text]: Print back the given text\n");
    printf(" - cls: Clear the screen\n");
}

void handle_echo(const char* input) {
    // Print everything after "echo "
    if (input[4] == ' ') {
        printf("%s\n", input + 5);
    } else {
        printf("Usage: echo [text]\n");
    }
}

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));
    
    HAL_Initialize();
    
    clrscr();
    
    printf("Hello from kernel!\n");
    printf("Type 'help' for a list of commands.\n\n");

    i686_IRQ_RegisterHandler(0, timer);
    i686_Keyboard_Initialize();

    char input_buffer[256];

    while (1) {
        printf("> ");
        gets(input_buffer, sizeof(input_buffer));

        if (strcmp(input_buffer, "help") == 0) {
            handle_help();
        } else if (strcmp(input_buffer, "cls") == 0) {
            clrscr();
        } else if (memcmp(input_buffer, "echo ", 5) == 0) {
            handle_echo(input_buffer);
        } else if (input_buffer[0] == '\0') {
            // Empty input, do nothing
        } else {
            printf("Unknown command: %s\n", input_buffer);
        }
    }

    // This part is now unreachable
    for (;;);
}
