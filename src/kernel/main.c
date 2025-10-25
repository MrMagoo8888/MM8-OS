#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include "fat.h"
#include <arch/i686/keyboard.h>
#include "string.h"

// --- Command History ---
#define HISTORY_SIZE 10
static char g_CommandHistory[HISTORY_SIZE][256];
static int g_HistoryCount = 0;
static int g_HistoryIndex = 0;

static DISK g_Disk;

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
    printf(" - read [file]: Read a file from the disk. Example: read /test.txt\n");
    printf(" - credits: Shows Credits from our Wonderful contributers and viewers\n");
}

void handle_echo(const char* input) {
    // Print everything after "echo "
    if (input[4] == ' ') {
        printf("%s\n", input + 5);
    } else {
        printf("Usage: echo [text]\n");
    }
}

void handle_read(const char* input) {
    if (input[4] != ' ') {
        printf("Usage: read [file]\n");
        return;
    }
    const char* path = input + 5;
    FAT_File* file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_READ);
    if (!file) {
        printf("Could not open file: %s\n", path);
        return;
    }

    char buffer[513]; // Read 512 bytes at a time
    uint32_t bytes_read;
    while ((bytes_read = FAT_Read(&g_Disk, file, 512, buffer)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    printf("\n");
    FAT_Close(&g_Disk, file);
}

void handle_writefile(const char* input) {
    if (input[5] != ' ') {
        printf("Usage: write [file]\n");
        return;
    }
    const char* path = input + 6;

    FAT_File* file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_WRITE);
    if (!file) {
        printf("Could not open file for writing: %s\n", path);
        return;
    }

    printf("Enter text. Type 'EOF' on a new line to save and exit.\n");

    char line_buffer[256];
    while (true) {
        gets(line_buffer, sizeof(line_buffer));
        if (strcmp(line_buffer, "EOF") == 0) {
            break;
        }
        FAT_Write(&g_Disk, file, strlen(line_buffer), line_buffer);
        FAT_Write(&g_Disk, file, 1, "\n"); // Write a newline character
    }

    FAT_Close(&g_Disk, file);
    printf("File saved.\n");
}

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
void credits() {
    printf("Credits to our Wonderful contributers and viewers:\n\n");
        printf(" - MrJBMG\n");
        printf(" - Una\n");
        printf(" - ChrisWestbro\n");
        printf(" - Bobby\n");
        //printf(" - \n");
        //printf(" - Viewer4\n");
        //printf(" - Viewer5\n");
        //printf(" - Viewer6\n");
        //printf(" - Viewer7\n");
        //printf(" - Viewer8\n");
        //printf(" - Viewer9\n");
        //printf(" - Viewer10\n");
        //printf(" - Viewer11\n");
        //printf(" - Viewer12\n");
        //printf(" - Viewer13\n");
        //printf(" - Viewer14\n");
}

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));
    
    HAL_Initialize();
    
    clrscr();
    
    printf("Hello from kernel!\n");
    printf("Type 'help' for a list of commands.\n\n");

    // Initialize disk and FAT filesystem
    // We are booting from floppy, but we want to use the first hard disk (0x80) for our root filesystem.
    if (!DISK_Initialize(&g_Disk, 0x80)) {
        printf("Hard disk initialization failed.\n");
    } else if (!FAT_Initialize(&g_Disk)) {
        printf("FAT initialization failed on hard disk.\n");
    }

    i686_IRQ_RegisterHandler(0, timer);
    i686_Keyboard_Initialize(g_CommandHistory, &g_HistoryCount, &g_HistoryIndex, HISTORY_SIZE);

    // Enable interrupts now that all handlers are set up
    i686_EnableInterrupts();

    char input_buffer[256];

    while (1) {
        printf("> ");
        gets(input_buffer, sizeof(input_buffer));
        add_to_history(input_buffer);

        if (strcmp(input_buffer, "help") == 0) {
            handle_help();
        } else if (strcmp(input_buffer, "cls") == 0) {
            clrscr();
        } else if (memcmp(input_buffer, "echo ", 5) == 0) {
            handle_echo(input_buffer);
        } else if (memcmp(input_buffer, "read ", 5) == 0) {
            handle_read(input_buffer);
        } else if (memcmp(input_buffer, "write ", 6) == 0) {
            handle_writefile(input_buffer);
        } else if (input_buffer[0] == '\0') {
            // Empty input, do nothing
        } else if (strcmp(input_buffer, "credits") == 0) {
            credits();
        } else {
            printf("Unknown command: %s\n", input_buffer);
        }
    
    }

    // This part is now unreachable
    for (;;);
}