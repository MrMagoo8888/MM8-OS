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

#define EDITOR_BUFFER_SIZE 4096
static char g_EditorBuffer[EDITOR_BUFFER_SIZE];

void redraw_editor(const char* buffer, int cursor_pos) {
    clrscr();
    // Print status bar first so it can be overwritten by text if needed
    printf("\n[Ctrl+S to Save and Exit]");
    setcursor(0, 0); // Reset cursor to top-left for drawing buffer
    printf("%s", buffer);
    
    // Calculate cursor X and Y from position
    int x = 0, y = 0;
    for (int i = 0; i < cursor_pos && buffer[i]; i++) {
        if (buffer[i] == '\n') {
            y++;
            x = 0;
        } else {
            x++;
            if (x >= SCREEN_WIDTH) {
                y++;
                x = 0;
            }
        }
    }
    setcursor(x, y);
}

void handle_editor(const char* input) {
    if (input[4] != ' ') {
        printf("Usage: edit [file]\n");
        return;
    }
    const char* path = input + 5;

    FAT_File* file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_CREATE);
    if (!file) {
        printf("Could not open or create file: %s\n", path);
        return;
    }

    // Read entire file into buffer
    uint32_t file_size = FAT_Read(&g_Disk, file, EDITOR_BUFFER_SIZE - 1, g_EditorBuffer);
    g_EditorBuffer[file_size] = '\0';
    FAT_Close(&g_Disk, file);

    int cursor_pos = file_size;
    redraw_editor(g_EditorBuffer, cursor_pos);

    while (true) {
        int key = getch();

        if (key == ('s' & 0x1F)) { // Ctrl+S
            file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_WRITE);
            if (file) {
                // Overwrite the file with the buffer content
                FAT_Write(&g_Disk, file, strlen(g_EditorBuffer), g_EditorBuffer);
                FAT_Close(&g_Disk, file);
                clrscr();
                printf("File saved.\n");
            } else {
                clrscr();
                printf("Error saving file.\n");
            }
            return;
        }

        switch (key) {
            case KEY_LEFT:
                if (cursor_pos > 0) cursor_pos--;
                break;
            case KEY_RIGHT:
                if (cursor_pos < strlen(g_EditorBuffer)) cursor_pos++;
                break;
            case KEY_UP:
            {
                int current_line_start = cursor_pos;
                while (current_line_start > 0 && g_EditorBuffer[current_line_start - 1] != '\n') {
                    current_line_start--;
                }
                if (current_line_start > 0) { // Not on the first line
                    int prev_line_end = current_line_start - 2;
                    int prev_line_start = prev_line_end;
                    while (prev_line_start > 0 && g_EditorBuffer[prev_line_start - 1] != '\n') {
                        prev_line_start--;
                    }
                    int col = cursor_pos - current_line_start;
                    cursor_pos = prev_line_start + col;
                    if (cursor_pos > prev_line_end + 1) cursor_pos = prev_line_end + 1;
                }
                break;
            }
            case KEY_DOWN:
                // This is a simplified implementation. A full one would find the start
                // of the next line and move to the same column, similar to KEY_UP.
                // For now, let's just move to the end of the buffer.
                cursor_pos = strlen(g_EditorBuffer);
                break;
            case '\b': // Backspace
                if (cursor_pos > 0) {
                    memmove(&g_EditorBuffer[cursor_pos - 1], &g_EditorBuffer[cursor_pos], strlen(g_EditorBuffer) - cursor_pos + 1);
                    cursor_pos--;
                }
                break;
            default:
                if (key >= 32 && key <= 126 && strlen(g_EditorBuffer) < EDITOR_BUFFER_SIZE - 2) {
                    memmove(&g_EditorBuffer[cursor_pos + 1], &g_EditorBuffer[cursor_pos], strlen(g_EditorBuffer) - cursor_pos + 1);
                    g_EditorBuffer[cursor_pos] = (char)key;
                    cursor_pos++;
                } else if (key == '\n' && strlen(g_EditorBuffer) < EDITOR_BUFFER_SIZE - 2) {
                    memmove(&g_EditorBuffer[cursor_pos + 1], &g_EditorBuffer[cursor_pos], strlen(g_EditorBuffer) - cursor_pos + 1);
                    g_EditorBuffer[cursor_pos] = '\n';
                    cursor_pos++;
                }
                break;
        }
        redraw_editor(g_EditorBuffer, cursor_pos);
    }
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
void credits() {
    printf("Credits to our Wonderful contributers and viewers:\n\n");
        printf(" - MrJBMG\n");
        printf(" - Una\n");
        printf(" - ChrisWestbro\n");
        printf(" - Bobby\n");
        printf(" - Alex\n");
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
        } else if (memcmp(input_buffer, "edit ", 5) == 0) {
            handle_editor(input_buffer);
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