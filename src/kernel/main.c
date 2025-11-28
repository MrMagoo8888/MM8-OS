// replace with older ver for test

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

void kernel_main(vbe_screen_t* vbe_info, uint32_t boot_drive) {
    // Cast the physical buffer address to a volatile pointer.
    volatile uint32_t* framebuffer = (volatile uint32_t*)vbe_info->physical_buffer;

    // Calculate the memory offset for the pixel at (x=20, y=20).
    uint32_t pixels_per_line = vbe_info->bytes_per_line / vbe_info->bytes_per_pixel;
    uint32_t pixel_offset = (20 * pixels_per_line) + 20;

    // Draw a bright green pixel.
    framebuffer[pixel_offset] = 0x0000FF00; // 0x00RRGGBB

    // Halt the system
    for (;;) {
        __asm__("hlt");
    }
}