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
#include <arch/i686/gdt.h>
#include "ctype.h"

#include "commands/credits.h"
#include "commands/cube.h"
#include "commands/color.h"
#include "heap.h"
#include "stdbool.h" // Required for 'bool' type

#include "threeD/rand1.h"

#include "time.h"

#include <apps/imageview/bmp.h>

// Simple function to run in Ring 3
static void user_mode_test() {
    const char* msg = "Hello from Ring 3 via System Call (0x80)!\n";

    // Trigger syscall: EAX=2 (puts), EBX=ptr
    __asm__ volatile("mov $2, %%eax\n\t"
                     "mov %0, %%ebx\n\t"
                     "int $0x80" 
                     : : "r"(msg) : "eax", "ebx");

    while (1);
}

static void handle_usermode() {
    printf("Switching to User Mode (Ring 3)...\n");

    // 1. Allocate a stack for the user process
    static uint8_t user_stack[4096];
    void* user_esp = &user_stack[4096]; 

    // 2. Tell the TSS where our Kernel Stack is.
    uint32_t kernel_esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(kernel_esp));
    i686_TSS_SetStack(i686_GDT_DATA_SEGMENT, kernel_esp);

    i686_EnterUserMode(user_mode_test, user_esp);
}

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
    printf(" - cube: Runs a 3D rotating cube test.\n");
    printf(" - memory: Show heap memory usage statistics.\n");
    printf(" - bmp [file]: View a BMP image file. Example: bmp /image.bmp (Work in Progress)\n");
    printf(" - uptime: Show the system uptime.\n");
    printf(" - usermode: Test switching the CPU to Ring 3 (User Mode)\n");
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

        // Filter out Long File Name (LFN) entries and Volume Labels
        // LFN entries have attributes 0x0F (Read Only | Hidden | System | Volume ID)
        if ((entry.Attributes & FAT_ATTRIBUTE_VOLUME_ID) || entry.Attributes == FAT_ATTRIBUTE_LFN) 
            continue;

        if (entry.Attributes & (FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM))
            continue;

        // Simple 8.3 name formatting
        char name[13];
        int pos = 0;
        for (int i = 0; i < 8 && entry.Name[i] != ' ' && entry.Name[i] != '\0'; i++) {
            if (isprint(entry.Name[i]))
                name[pos++] = entry.Name[i];
        }

        if (entry.Name[8] != ' ') {
            name[pos++] = '.';
            for (int i = 8; i < 11 && entry.Name[i] != ' ' && entry.Name[i] != '\0'; i++) {
                if (isprint(entry.Name[i]))
                    name[pos++] = entry.Name[i];
            }
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
    const char* arg = input + 4;
    while (*arg == ' ') arg++;
    if (*arg != '\0') {
        printf("%s\n", arg);
    } else {
        printf("Usage: echo [text]\n");
    }
}

static void handle_read(const char* input) {
    const char* path = input + 4;
    while (*path == ' ') path++;
    
    if (*path == '\0') {
        printf("Usage: read [file]\n");
        return;
    }

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

static void handle_memory() {
    size_t total, used, free_mem;
    heap_get_stats(&total, &used, &free_mem);
    printf("Heap Statistics:\n");
    printf("  Total Size: %u bytes (%u MB)\n", total, total / 1024 / 1024);
    printf("  Used:       %u bytes\n", used);
    printf("  Free:       %u bytes\n", free_mem);
    printf("  Overhead:   %u bytes\n", total - used - free_mem);
}

static bool handle_exec(const char* path) {
    FAT_File* file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_READ);
    if (!file) {
        return false;
    }

    // 1. Allocate memory for the program and a stack
    // In a real OS, we'd use paging to map this to a specific user address
    uint8_t* program_buffer = (uint8_t*)malloc(file->Size);
    uint8_t* user_stack = (uint8_t*)malloc(4096);

    if (!program_buffer || !user_stack) {
        printf("Exec: Memory allocation failed\n");
        if (program_buffer) free(program_buffer);
        FAT_Close(&g_Disk, file);
        return false;
    }

    // 2. Load the binary
    FAT_Read(&g_Disk, file, file->Size, program_buffer);
    FAT_Close(&g_Disk, file);

    // 3. Prepare for the jump
    uint32_t kernel_esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(kernel_esp));
    i686_TSS_SetStack(i686_GDT_DATA_SEGMENT, kernel_esp);

    printf("Executing %s in Ring 3...\n", path);
    i686_EnterUserMode((void(*)())program_buffer, user_stack + 4096);

    return true;
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

void command_dispatch(const char* input) {
    if (input[0] == '\0') {
        return;
    }

    // 1. Attempt to execute as a Ring 3 binary from disk
    char path[64] = "/";
    strcpy(path + 1, input);
    if (handle_exec(path)) {
        return;
    }

    // 2. Fallback to pre-existing built-in commands
    if (strcmp(input, "help") == 0) {
        handle_help();
    } else if (strcmp(input, "ls") == 0) {
        handle_ls();
    } else if (strcmp(input, "cls") == 0) {
        clrscr();
    } else if (memcmp(input, "echo", 4) == 0 && (input[4] == ' ' || input[4] == '\0')) {
        handle_echo(input);
    } else if (memcmp(input, "read", 4) == 0 && (input[4] == ' ' || input[4] == '\0')) {
        handle_read(input);
    } else if (memcmp(input, "edit", 4) == 0 && (input[4] == ' ' || input[4] == '\0')) {
        editor_handle_command(input);
    } else if (strcmp(input, "afk") == 0) {
        afk();
    } else if (strcmp(input, "uptime") == 0) {
        handleUptime();
    } else if (strcmp(input, "json_test") == 0) {
        handle_json_test();
    } else if (memcmp(input, "color ", 6) == 0) {
        handle_color(input);
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
    } else if (strcmp(input, "usermode") == 0) {
        handle_usermode();
    } else if (memcmp(input, "fontsize ", 9) == 0) {
        handle_fontsize(input);
    } else if (strcmp(input, "cube") == 0) {
        cube_test();
    } else if (memcmp(input, "bmp", 3) == 0 && (input[3] == ' ' || input[3] == '\0')) {
        const char* arg = input + 3;
        while (*arg == ' ') arg++; // Skip extra spaces
        if (*arg == '\0') {
            printf("Usage: bmp [file]\n");
        } else {
            bmp_view(arg);
        }
    } else if (strcmp(input, "memory") == 0) { // This was in the original, keep it.
        handle_memory();
    } else {
        // 3. If neither Ring 3 nor built-in, then it's truly unknown
        printf("Unknown command: %s\n", input);
    }
}