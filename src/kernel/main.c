#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include <arch/i686/keyboard.h> // This include is already present, but good to confirm
#include "disk.h"
#include "ff.h"
#include <arch/i686/keyboard.h>
#include "string.h"
#include "heap.h"
#include <commands/command.h>
#include <apps/editor/editor.h>
#include "globals.h"
#include <apps/calc/calc.h> // Include for handle_calc
#include "vbe.h"
#include "graphics.h"
#include <apps/imageview/bmp.h>
#include "gdt.h"
#include "vfs.h"
#include "fat.h"


DISK g_Disk;
FATFS g_fs;

// Add a global tick counter, updated by the timer IRQ
volatile uint32_t g_ticks = 0;

// Define the global pointer to the VBE info structure.
VbeScreenInfo* g_vbe_screen;

// Create a static instance to hold the VBE info passed from the bootloader.
static VbeScreenInfo s_vbe_screen;

extern uint8_t __bss_start;
extern uint8_t __end;

void timer(Registers* regs)
{
    g_ticks++;
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

void __attribute__((section(".entry"))) start(VbeScreenInfo* vbe_info, uint16_t bootDrive)
{
    // Crash the system to verify we've reached the kernel.
    // __asm__ volatile ("int $0x3"); // Ensure this is commented out!
    
    // Clear BSS first. This must happen before we copy data into static variables.
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    // Copy the VBE info from the bootloader to a safe location in the kernel's memory.
    if (vbe_info) {
        memcpy(&s_vbe_screen, vbe_info, sizeof(VbeScreenInfo));
    }

    // Now that BSS is clear, we can safely initialize our global variables.
    g_vbe_screen = &s_vbe_screen;

    gdt_initialize();
    HAL_Initialize();

    heap_initialize();
    console_initialize();
    
    clrscr();
    
    printf("    ==================================================================\n");
    printf("\n"
           "    '##::::'##:'##::::'##::'#######::::::::::::'#######:::'######::\n"
           "    ###::'###: ###::'###:'##.... ##::::::::::'##.... ##:'##... ##:\n"
           "    ####'####: ####'####: ##:::: ##:::::::::: ##:::: ##: ##:::..::\n"
           "    ## ### ##: ## ### ##:: #######::'#######: ##:::: ##:. ######::\n"
           "    ##. #: ##: ##. #: ##:'##.... ##:........: ##:::: ##::..... ##:\n"
           "    ##:.:: ##: ##:.:: ##: ##:::: ##:::::::::: ##:::: ##:'##::: ##:\n"
           "    ##:::: ##: ##:::: ##:. #######:::::::::::. #######::. ######::\n"
           "    ..:::::..::..:::::..:::.......:::::::::::::.......::::......:::\n");
    printf("\n");
    printf("    ==================================================================\n");




    printf("\n\nType 'help' for a list of commands.\n\n");

    // Initialize FatFs
    FRESULT res = f_mount(&g_fs, "0:", 1);
    if (res != FR_OK) {
        printf("FatFs mount failed with error code: %d\n", res);
    } else {
        printf("FatFs mounted successfully.\n");
    }
    
    // Initialize VFS and mount FatFs driver to root
    VFS_Initialize();
    VFS_Mount("/", &g_FatFsDriver);

    i686_IRQ_RegisterHandler(0, timer);
    i686_Keyboard_Initialize(g_CommandHistory, &g_HistoryCount, &g_HistoryIndex, HISTORY_SIZE);

    // Enable interrupts now that all handlers are set up
    i686_EnableInterrupts();

    char input_buffer[256];

    while (1) {
        printf("> ");


        gets(input_buffer, sizeof(input_buffer));

        add_to_history(input_buffer);


        command_dispatch(input_buffer); // invokes commands like help, cls, echo, read, edit
    
    }

    for (;;);
}