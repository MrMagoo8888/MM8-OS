#include "command.h"
#include "stdio.h"
#include "string.h"
#include "fat.h"
#include <apps/editor/editor.h>
#include "memory.h"
#include "globals.h"
#include "json.h"
#include "afk.h"
#include <apps/calc/calc.h>

#include "commands/credits.h"

// Command handler functions (made static as they are internal to this file)
static void handle_help() {
    printf("Available commands:\n");
    printf(" - help: Show this message\n");
    printf(" - echo [text]: Print back the given text\n");
    printf(" - cls: Clear the screen\n");
    printf(" - read [file]: Read a file from the disk. Example: read /test.txt\n");
    printf(" - edit [file]: Open or create a file for editing.\n");
    printf(" - credits: Shows Credits from our Wonderful contributers and viewers\n");
    printf(" - json_test: Runs a test of the cJSON library and heap.\n");
}

static void handle_echo(const char* input) {
    // Print everything after "echo "
    if (input[4] == ' ') {
        printf("%s\n", input + 5);
    } else {
        printf("Usage: echo [text]\n");
    }
}

static void handle_read(const char* input) {
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

void command_dispatch(const char* input) {
    if (strcmp(input, "help") == 0) {
        handle_help();
    } else if (strcmp(input, "cls") == 0) {
        clrscr();
    } else if (memcmp(input, "echo ", 5) == 0) {
        handle_echo(input);
    } else if (memcmp(input, "read ", 5) == 0) {
        handle_read(input);
    } else if (memcmp(input, "edit ", 5) == 0) {
        editor_handle_command(input);
    } else if (strcmp(input, "afk") == 0) {
        afk();
    } else if (input[0] == '\0') {
        // Empty input, do nothing
    } else if (strcmp(input, "credits") == 0) {
        credits();
    } else if (memcmp(input, "json_test", 9) == 0) {
        handle_json_test();
    } else if (memcmp(input, "calc", 4) == 0 && (input[4] == ' ' || input[4] == '\0')) {
        handle_calc(input);
    } else {
        printf("Unknown command: %s\n", input);
    }
}