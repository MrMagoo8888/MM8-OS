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

void __attribute__((section(".entry"))) start(uint16_t bootDrive, vbe_screen_t* vbe_info)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    // Initialize hardware and interrupts first to prevent triple faults
    HAL_Initialize();
    i686_IRQ_RegisterHandler(0, timer);
    i686_Keyboard_Initialize(g_CommandHistory, &g_HistoryCount, &g_HistoryIndex, HISTORY_SIZE);
    i686_EnableInterrupts();

    g_vbe_screen_info = vbe_info;
    
    heap_initialize(vbe_info);
    
    // Clear the screen to a nice pink color
    graphics_clear_screen(0xFFFF5486);
    
    // mm8Splash(); // This will no longer work as it uses text-mode printf

    // Let's draw something to test!
    graphics_draw_rect(100, 100, 200, 150, 0xFF00FF00); // A green rectangle
    graphics_draw_rect(120, 120, 160, 110, 0xFFFF0000); // A red rectangle inside
    graphics_draw_line(0, 0, g_vbe_screen_info->width - 1, g_vbe_screen_info->height - 10, 0xFFFFFFFF); // A white diagonal line
    
    // Let's test our new graphics text functions!
    graphics_draw_string(20, 20, "MM8-OS Graphical Mode!", 0xFFFFFFFF);
    graphics_draw_string(20, 30, "----------------------", 0xFFFFFFFF);
    
    char buffer[256];
    sprintf(buffer, "Resolution: %dx%d %dbpp", g_vbe_screen_info->width, g_vbe_screen_info->height, g_vbe_screen_info->bpp);
    graphics_draw_string(20, 50, buffer, 0xFF00FFFF); // Cyan
    
    sprintf(buffer, "Framebuffer: 0x%x", g_vbe_screen_info->physical_buffer);
    graphics_draw_string(20, 60, buffer, 0xFF00FFFF); // Cyan
    
    graphics_draw_string(20, 80, "Type 'help' for a list of commands.", 0xFFFFFF00); // Yellow

    // Initialize disk and FAT filesystem
    // We are booting from floppy, but we want to use the first hard disk (0x80) for our root filesystem.
    if (!DISK_Initialize(&g_Disk, 0x80)) {
        printf("Hard disk initialization failed.\n");
    } else if (!FAT_Initialize(&g_Disk)) {
        printf("FAT initialization failed on hard disk.\n");
    }

    // This will go to serial, but our graphical text works!
    printf("MM8-OS Kernel Started.\n");

    // The main loop is now empty. We've drawn our graphics and will halt.
    // In the future, this will be the event loop for the GUI.

    // This part is now unreachable
    for (;;);
}