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
#include "graphics.h"
#include <apps/imageview/bmp.h>
#include "time.h" // Include the new time.h header
#include <misc/noCrash.h>


DISK g_Disk;

void pci_enumerate();

// Add a global tick counter, updated by the timer IRQ
volatile uint32_t g_ticks = 0;

// Define the global pointer to the VBE info structure.
VbeScreenInfo* g_vbe_screen;

// Track the current directory path for the shell prompt
char g_CurrentPath[256] = "/";

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

void shell_readline(char* buffer, int buffer_size) {
    int pos = 0;
    int history_view_index = -1; // -1 means we are not currently viewing history
    char current_line_backup[256] = "";

    while (1) {
        int c = getch();

        if (c == '\n' || c == '\r') {
            buffer[pos] = '\0';
            putc('\n');
            break;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                putc('\b');
            }
        } else if (c == KEY_UP) {
            if (g_HistoryCount > 0) {
                if (history_view_index == -1) {
                    buffer[pos] = '\0';
                    strcpy(current_line_backup, buffer);
                    history_view_index = g_HistoryCount - 1;
                } else if (history_view_index > 0) {
                    history_view_index--;
                }
                while (pos > 0) { putc('\b'); pos--; }
                int idx = (g_HistoryIndex - g_HistoryCount + history_view_index + HISTORY_SIZE) % HISTORY_SIZE;
                strcpy(buffer, g_CommandHistory[idx]);
                pos = strlen(buffer);
                puts(buffer);
            }
        } else if (c == KEY_DOWN) {
            if (history_view_index != -1) {
                while (pos > 0) { putc('\b'); pos--; }
                if (history_view_index < g_HistoryCount - 1) {
                    history_view_index++;
                    int idx = (g_HistoryIndex - g_HistoryCount + history_view_index + HISTORY_SIZE) % HISTORY_SIZE;
                    strcpy(buffer, g_CommandHistory[idx]);
                } else {
                    history_view_index = -1;
                    strcpy(buffer, current_line_backup);
                }
                pos = strlen(buffer);
                puts(buffer);
            }
        } else if (c == KEY_PAGE_UP) {
            for (int i = 0; i < 5; i++) view_scrollback_up();
        } else if (c == KEY_PAGE_DOWN) {
            for (int i = 0; i < 5; i++) view_scrollback_down();
        } else if (c >= 32 && c <= 126 && pos < buffer_size - 1) {
            buffer[pos++] = (char)c;
            putc((char)c);
        }
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

    HAL_Initialize();
    heap_initialize();
    init_tests(); 
    console_initialize();
    time_initialize();
    
    // Set up core interrupts before running tests or shell
    i686_IRQ_RegisterHandler(0, timer);
    i686_Keyboard_Initialize(g_CommandHistory, &g_HistoryCount, &g_HistoryIndex, HISTORY_SIZE);
    i686_EnableInterrupts();

    clrscr();

    // Now that interrupts are enabled, PCI enumeration (and OHCI sleep) will work
    pci_enumerate();
    
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


    if (!DISK_Initialize(&g_Disk, 0x80)) {
        printf("System disk initialization failed (No ATA or USB drive found).\n");
    } else if (!FAT_Initialize(&g_Disk)) {
        printf("FAT initialization failed on hard disk.\n");
    }

    char input_buffer[256];

    while (1) {
        // Classic Linux-style prompt: mm8@mm8-os:/$ 
        printf("mm8@mm8-os:%s$ ", g_CurrentPath);

        shell_readline(input_buffer, sizeof(input_buffer));

        add_to_history(input_buffer);


        command_dispatch(input_buffer); // invokes commands like help, cls, echo, read, edit
    
    }

    for (;;);
}