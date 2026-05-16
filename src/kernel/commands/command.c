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
#include "mm8Splash.h"

#include "commands/bot.h"
#include "commands/credits.h"
#include "commands/cube.h"
#include "commands/color.h"
#include "stdlib.h"
#include "ctype.h"
#include "heap.h"

#include <apps/gameEngine/gameEngine.h>

#include "threeD/rand1.h"
#include <arch/i686/gdt.h> // For i686_EnterUserMode

#include "time.h"

#include <apps/imageview/bmp.h>

// Prototypes for HAL functions
void HAL_Speaker_Play(uint32_t frequency);
void HAL_Speaker_Stop();

// Command handler functions (made static as they are internal to this file)
static void handle_help() {

    clrscr();

    mm8Splash(); // this now works, noneed to comment out

    printf("Available commands:\n\n");
    printf(" - help: Show this message\n");
    printf(" - ls: List files in the root directory\n");
    printf(" - echo [text]: Print back the given text\n");
    printf(" - cls: Clear the screen\n");
    printf(" - afk: Activate AFK mode with a screensaver\n");
    printf(" - color [color or Hexadecimal]: Change text color.\n");
    printf(" - read [file]: Read a file from the disk. Example: read /test.txt\n");
    printf(" - edit [file]: Open or create a file for editing.\n");
    printf(" - credits: Shows Credits from our Wonderful contributers and viewers\n");
    printf(" - fontsize [1-9]: Change the font scale.\n");
    printf(" - json_test: Runs a test of the cJSON library and heap.\n");
    printf(" - bot [query]: Talk to the MM8 Assistant bot.\n");
    printf(" - cube: Runs a 3D rotating cube test.\n");
    printf(" - memory: Show heap memory usage statistics.\n");
    printf(" - bmp [file]: View a BMP image file. Example: bmp /image.bmp (Work in Progress)\n");
    printf(" - uptime: Show the system uptime.\n");
    printf(" - beep [freq]: Play a sound at the specified frequency.\n");
    printf(" - play [freq:ms,...]: Play a sequence of notes. Example: play 440:200,880:100\n");
    printf(" - jingle: Play a short melody.\n");
}

static void handle_ls() {
    FAT_File* root = &g_Data->RootDirectory.Public;
    if (!FAT_Seek(&g_Disk, root, 0)) {
        printf("ls: Failed to seek root directory\n");
        return;
    }

    FAT_DirectoryEntry entry;
    printf("Directory of /\n\n");
    
    while (FAT_ReadEntry(&g_Disk, root, &entry)) {
        if (entry.Name[0] == 0x00) break;   // End of directory
        if (entry.Name[0] == 0xE5) continue; // Deleted entry
        if (entry.Attributes & FAT_ATTRIBUTE_VOLUME_ID) continue;

        // Simple 8.3 name formatting
        char name[13];
        int pos = 0;
        for (int i = 0; i < 8 && entry.Name[i] != ' '; i++) name[pos++] = entry.Name[i];
        if (entry.Name[8] != ' ') {
            name[pos++] = '.';
            for (int i = 8; i < 11 && entry.Name[i] != ' '; i++) name[pos++] = entry.Name[i];
        }
        name[pos] = '\0';

        if (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) {
            printf("  [DIR] %s\n", name);
        } else {
            printf("  %8u  %s\n", entry.Size, name);
        }
    }
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

static void handle_fontsize(const char* input) {
    // Expected format: "fontsize <scale>"
    // Skip "fontsize " (9 chars)
    const char* arg = input + 9;
    if (*arg >= '1' && *arg <= '9') {
        int scale = *arg - '0';
        console_set_font_scale(scale);
        printf("Font scale set to %d\n", scale);
    } else {
        printf("Usage: fontsize <1-9>\n");
    }
}

static void handle_typewriter(const char* input) {
    const char* arg = input + 11;
    while (*arg == ' ') arg++;
    
    uint32_t delay = (uint32_t)atoi(arg);
    g_ConsoleDelay = delay;
    printf("Typewriter delay set to %u ms (0 = Fast/Batched)\n", delay);
}

static void handle_memory() {
    size_t total, used, free_mem;
    heap_get_stats(&total, &used, &free_mem);
    printf("Heap Statistics:\n");
    printf("  Total Size: %u bytes (%u MB)\n", total, total / 1024 / 1024);
    printf("  Used:       %u bytes\n", used);
    printf("  Free:       %u bytes\n", free_mem);
    printf("  Overhead:   %u bytes\n", total - used - free_mem);
}

void handleUptime() {
    uint32_t ms = get_uptime_ms();
    uint32_t seconds = ms / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;
    printf("System Uptime Statistics:\n");
    printf("  Total: %lu ms\n", ms);
    printf("  Clock: %lu days, %02lu:%02lu:%02lu\n", days, hours % 24, minutes % 60, seconds % 60);
}

static void handle_beep(const char* input) {
    uint32_t freq = 440; // Default A4
    if (memcmp(input, "beep ", 5) == 0) {
        freq = (uint32_t)atoi(input + 5);
    }
    
    if (freq == 0) freq = 440;

    HAL_Speaker_Play(freq);
    sleep_ms(200);
    HAL_Speaker_Stop();
}

static void handle_jingle() {
    // Frequencies: C5, E5, G5, C6
    uint32_t melody[] = { 523, 659, 784, 1046 };
    uint32_t durations[] = { 100, 100, 100, 300 };

    printf("Playing jingle...\n");

    for (int i = 0; i < 4; i++) {
        HAL_Speaker_Play(melody[i]);
        sleep_ms(durations[i]);
        HAL_Speaker_Stop();
        // A very short pause between notes makes it sound much clearer
        sleep_ms(20); 
    }
}

static void handle_play(const char* input) {
    const char* ptr = input + 5; // Skip "play "
    
    while (*ptr) {
        // Skip leading whitespace or commas
        while (*ptr == ' ' || *ptr == ',') ptr++;
        if (!*ptr) break;

        if (!isdigit(*ptr)) break;

        // Parse frequency
        uint32_t freq = (uint32_t)atoi(ptr);
        while (*ptr && isdigit(*ptr)) ptr++;

        if (*ptr != ':') {
            printf("Error: Expected ':' after frequency\n");
            return;
        }
        ptr++; // skip ':'

        // Parse duration
        uint32_t ms = (uint32_t)atoi(ptr);
        while (*ptr && isdigit(*ptr)) ptr++;

        // Play the note
        HAL_Speaker_Play(freq);
        sleep_ms(ms);
        HAL_Speaker_Stop();
        sleep_ms(10); // Tiny silence for note clarity
    }
}

// Defined in main.c
extern void user_mode_test_program();

/*void handle_usermode_test() {
    printf("Attempting to jump to User Mode...\n");

    // Allocate a separate stack for the user program
    void* user_stack = malloc(4096);
    if (!user_stack) {
        printf("Error: Failed to allocate user stack.\n");
        return;
    }
    void* user_stack_top = (uint8_t*)user_stack + 4096;
    i686_EnterUserMode(user_mode_test_program, user_stack_top);
}
*/

void command_dispatch(const char* input) {
    if (input[0] == '\0')
        return;

    if (strcmp(input, "help") == 0) {
        handle_help();
    } else if (strcmp(input, "ls") == 0) {
        handle_ls();
    } else if (strcmp(input, "cls") == 0) {
        clrscr();
    } else if (memcmp(input, "echo ", 5) == 0) {
        handle_echo(input);
    } else if (memcmp(input, "read ", 5) == 0) {
        handle_read(input);
    } else if (memcmp(input, "edit ", 5) == 0) {
        editor_handle_command(input);
    } else if (memcmp(input, "beep", 4) == 0) {
        handle_beep(input);
    } else if (memcmp(input, "play ", 5) == 0) {
        handle_play(input);
    } else if (strcmp(input, "jingle") == 0) {
        handle_jingle();
    } else if (strcmp(input, "game_test") == 0) {
        game_engine_run();
    } else if (strcmp(input, "afk") == 0) {
        afk();
    } else if (strcmp(input, "uptime") == 0) {
        handleUptime();
    } else if (strcmp(input, "json_test") == 0) {
        handle_json_test();
    } else if (memcmp(input, "color ", 6) == 0) {
        handle_color(input);
    } else if (memcmp(input, "bot", 3) == 0 && (input[3] == ' ' || input[3] == '\0')) {
        handle_bot(input);
    } else if (input[0] == '\0') {
        // Empty input, do nothing
    } else if (strcmp(input, "credits") == 0) {
        credits();
    } else if (strcmp(input, "mm8Splash") == 0) {
        mm8Splash();
    } else if (strcmp(input, "rand1") == 0) {
        handle_rand1();
    } else if (strcmp(input, "rand2") == 0) {
        handle_rand2();
    } else if (strcmp(input, "rand3") == 0) {
        handle_rand3();
    } else if (strcmp(input, "rand4") == 0) {
        handle_rand4();
    } else if (memcmp(input, "calc", 4) == 0 && (input[4] == ' ' || input[4] == '\0')) {
        handle_calc(input);
    } else if (memcmp(input, "fontsize ", 9) == 0) {
        handle_fontsize(input);
    } else if (memcmp(input, "typewriter", 10) == 0) {
        handle_typewriter(input);
    } else if (strcmp(input, "cube") == 0) {
        cube_test();
    } else if (strcmp(input, "usermode_test") == 0) {
        //handle_usermode_test();
        printf("User mode test has been decapitated.\n");
    } else if (memcmp(input, "bmp", 3) == 0 && (input[3] == ' ' || input[3] == '\0')) {
        const char* arg = input + 3;
        while (*arg == ' ') arg++; // Skip extra spaces
        if (*arg == '\0') {
            printf("Usage: bmp [file]\n");
        } else {
            bmp_view(arg);
        }
    } else if (strcmp(input, "memory") == 0) {
        handle_memory();
    } else {
        printf("Unknown command: %s\n", input);
    
    }
}