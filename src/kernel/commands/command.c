#include "command.h"
#include "stdio.h"
#include "graphics.h"
#include "string.h"
#include "fat.h"
#include <apps/editor/editor.h>
#include "memory.h"
#include "globals.h"
#include "json.h"
#include "afk.h"
#include <apps/calc/calc.h>
#include "mm8Splash.h"
//#include "3dAniTest.h"

#include "commands/color.h"
#include "commands/credits.h"

// Command handler functions (made static as they are internal to this file)
static void handle_help() {
    graphics_clear_screen(0xFFFF5486);
    int y = 20;
    graphics_draw_string(20, y, "MM8-OS Help", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "-----------", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- help: Show this message", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- cls: Clear the screen", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- echo [text]: Print back the given text", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- read [file]: Read a file from the disk", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- edit [file]: Open or create a file", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- credits: Show credits", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- json_test: Run cJSON heap test", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- calc: Open the calculator", 0xFFFFFFFF); y += 10;
    graphics_draw_string(20, y, "- afk: Display AFK screen", 0xFFFFFFFF); y += 10;
}

static void handle_echo(const char* input) {
    // Print everything after "echo "
    if (input[4] == ' ') {
        // For now, draw at a fixed position. A scrolling console is needed for more.
        graphics_draw_string(20, 120, input + 5, 0xFFFFFFFF);
    } else {
        graphics_draw_string(20, 120, "Usage: echo [text]", 0xFFFFFFFF);
    }
}

static void handle_read(const char* input) {
    // For now, draw at a fixed position.
    int y = 120;

    if (input[4] != ' ') {
        graphics_draw_string(20, y, "Usage: read [file]", 0xFFFFFFFF);
        return;
    }
    const char* path = input + 5;
    FAT_File* file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_READ);
    if (!file) {
        graphics_draw_string(20, y, "Could not open file.", 0xFFFF0000);
        return;
    }
    
    graphics_clear_screen(0xFFFF5486); // Clear screen to show file content
    char buffer[513]; // Read 512 bytes at a time
    uint32_t bytes_read;
    while ((bytes_read = FAT_Read(&g_Disk, file, 512, buffer)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    printf("\n");
    FAT_Close(&g_Disk, file); // NOTE: The printf calls inside the loop are still an issue.
}

void command_dispatch(const char* input) {
    if (strcmp(input, "help") == 0) {
        handle_help();
    } else if (strcmp(input, "cls") == 0) {
        graphics_clear_screen(0xFFFF5486);
    } else if (memcmp(input, "echo ", 5) == 0) {
        handle_echo(input);
    } else if (memcmp(input, "read ", 5) == 0) {
        handle_read(input);
    } else if (memcmp(input, "edit ", 5) == 0) {
        editor_handle_command(input);
    } else if (strcmp(input, "afk") == 0) {
        afk();
    //} else if (strcmp(input, "3dani_test") == 0) {
    //    ani3d_test();
    } else if (memcmp(input, "color ", 6) == 0) {
        handle_color(input);
    } else if (input[0] == '\0') {
        // Empty input, do nothing
    } else if (strcmp(input, "credits") == 0) {
        credits();
    } else if (memcmp(input, "json_test", 9) == 0) {
        handle_json_test();
    } else if (memcmp(input, "calc", 4) == 0 && (input[4] == ' ' || input[4] == '\0')) {
        handle_calc(input);
    } else {
        graphics_draw_string(20, 120, "Unknown command.", 0xFFFF0000);
    }
}