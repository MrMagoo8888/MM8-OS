#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include <arch/i686/keyboard.h> // This include is already present, but good to confirm
#include "fat.h"
#include <arch/i686/keyboard.h>
#include "string.h"
#include "heap.h"
#include <commands/command.h>
#include <apps/editor/editor.h>
#include "globals.h"
#include <apps/calc/calc.h> // Include for handle_calc
#include "vbe.h"
#include <commands/mm8Splash.h>
#include "graphics.h"


DISK g_Disk;
vbe_screen_t* g_vbe_screen_info;


extern uint8_t __bss_start;
extern uint8_t __end;

void timer(Registers* regs)
{
    // This function is called by the system timer (IRQ 0).
    // For now, it does nothing but is required to acknowledge the interrupt.
}

#define HISTORY_SIZE 10 // Define the size of the command history
static char g_CommandHistory[HISTORY_SIZE][256];
static int g_HistoryCount = 0;
static int g_HistoryIndex = 0;

void add_to_history(const char* command) {
    if (command[0] == '\0') return; // Don't save empty commands
    // Don't save if it's the same as the last command
    if (g_HistoryCount > 0 && strcmp(command, g_CommandHistory[(g_HistoryIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE]) == 0) {
        return;
    }
    strcpy(g_CommandHistory[g_HistoryIndex], command);
    g_HistoryIndex = (g_HistoryIndex + 1) % HISTORY_SIZE;
    if (g_HistoryCount < HISTORY_SIZE) {
        g_HistoryCount++;
    }
}

// Your kernel's entry point
// The arguments are on the stack, so we can access them as function parameters.
void __attribute__((section(".entry"))) start(uint16_t bootDrive, vbe_screen_t* vbe_info)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    // Cast the framebuffer address to a pointer we can use
    // Assuming vbe_info->physical_buffer points to the start of the framebuffer
    unsigned int* framebuffer = (unsigned int*)vbe_info->physical_buffer;
    
    // Calculate the total number of pixels on the screen
    unsigned int total_pixels = vbe_info->width * vbe_info->height;
    
    // Define a color (e.g., blue in 0x00RRGGBB format)
    unsigned int blue = 0x000000FF; // ARGB, so 0x00RRGGBB

    // Loop through every pixel and set it to our color
    for (unsigned int i = 0; i < total_pixels; i++) {
        framebuffer[i] = blue;
    }
    
    // At this point, the screen should be blue.
    // We halt the CPU to prevent it from executing random memory.
    for (;;) {
        __asm__("hlt");
    }
}