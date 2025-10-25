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

#define EDITOR_BUFFER_SIZE 4096
static char g_EditorBuffer[EDITOR_BUFFER_SIZE];

void redraw_editor(const char* buffer, int cursor_pos) {
    clrscr();
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

    printf("\n[Ctrl+S to Save and Exit]");
    setcursor(0, SCREEN_HEIGHT - 1);

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
            // UP and DOWN are complex, involving finding previous/next lines.
            // We'll leave them for a future enhancement.
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
        printf("\n[Ctrl+S to Save and Exit]");
        setcursor(0, SCREEN_HEIGHT - 1);
    }
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