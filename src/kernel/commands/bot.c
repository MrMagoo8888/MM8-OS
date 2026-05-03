#include "commands/bot.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "memory.h"
#include "heap.h"

void handle_bot(const char* input) {
    // Skip "bot " if it exists
    const char* query = input;
    if (memcmp(input, "bot ", 4) == 0) {
        query = input + 4;
    } else {
        printf("Bot: Hello! I am the MM8-OS Assistant. Try 'bot status' or 'bot hello'.\n");
        return;
    }

    if (strcmp(query, "hello") == 0 || strcmp(query, "hi") == 0) {
        printf("Bot: Greetings, MM8 user! I'm running deep inside the kernel.\n");
    } 
    else if (strcmp(query, "status") == 0) {
        size_t total, used, free_mem;
        heap_get_stats(&total, &used, &free_mem);
        printf("Bot: Systems are nominal.\n");
        printf("     Memory: %u%% used.\n", (used * 100) / total);
        printf("     Uptime: %u ms.\n", get_uptime_ms());
    }
    else if (strcmp(query, "uptime") == 0) {
        printf("Bot: We have been alive for %u milliseconds!\n", get_uptime_ms());
    }
    else if (strcmp(query, "who") == 0) {
        printf("Bot: I am a helper bot created to demonstrate shell extensions in MM8-OS.\n");
    }
    else {
        printf("Bot: I'm sorry, I don't know how to '%s' yet. I'm still learning!\n", query);
    }
}