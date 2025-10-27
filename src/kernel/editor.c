#include "editor.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"
#include "fat.h"
#include <arch/i686/keyboard.h>
#include "globals.h"

#define EDITOR_BUFFER_SIZE 4096
static char g_EditorBuffer[EDITOR_BUFFER_SIZE];

static void redraw_editor(const char* buffer, int cursor_pos) {
    clrscr();
    // Print status bar first so it can be overwritten by text if needed
    printf("\n[Ctrl+S to Save and Exit]\n\n\n");
    setcursor(0, 0); // Reset cursor to top-left for drawing buffer
    printf("%s", buffer);
    
    // Calculate cursor X and Y from position
    int x = 0, y = 4;
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

void editor_handle_command(const char* input) {
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
                // Find the start of the current line
                int current_line_start = cursor_pos;
                while (current_line_start > 0 && g_EditorBuffer[current_line_start - 1] != '\n') current_line_start--;
                int col = cursor_pos - current_line_start;

                // Find the start of the previous line
                if (current_line_start > 0) {
                    int prev_line_end = current_line_start - 2; // Position of char before the '\n'
                    int prev_line_start = prev_line_end;
                    while (prev_line_start > 0 && g_EditorBuffer[prev_line_start - 1] != '\n') prev_line_start--;
                    
                    // Move to the same column on the previous line, or the end of it
                    cursor_pos = prev_line_start + col;
                    if (cursor_pos > prev_line_end + 1) cursor_pos = prev_line_end + 1;
                }
                break;
            }
            case KEY_DOWN:
            {
                // Find the start and end of the current line
                int current_line_start = cursor_pos;
                while (current_line_start > 0 && g_EditorBuffer[current_line_start - 1] != '\n') current_line_start--;
                int col = cursor_pos - current_line_start;

                int current_line_end = cursor_pos;
                while(g_EditorBuffer[current_line_end] != '\0' && g_EditorBuffer[current_line_end] != '\n') current_line_end++;

                // Find the start and end of the next line
                if (g_EditorBuffer[current_line_end] == '\n') {
                    int next_line_start = current_line_end + 1;
                    int next_line_end = next_line_start;
                    while(g_EditorBuffer[next_line_end] != '\0' && g_EditorBuffer[next_line_end] != '\n') next_line_end++;
                    cursor_pos = next_line_start + col;
                    if (cursor_pos > next_line_end) cursor_pos = next_line_end;
                }
                break;
            }
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