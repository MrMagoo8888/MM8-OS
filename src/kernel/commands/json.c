#include "json.h"
#include "stdio.h"
#include "fat.h"
#include "heap.h"
#include "memory.h"
//#include "../src/kernel/libs/cjson/cJSON.h"
#include <libs/cJSON/cJSON.h>
#include <arch/i686/keyboard.h>

void handle_json_test() {
    printf("Running cJSON test...\n");

    FAT_File* file = FAT_Open(&g_Disk, "/test.jsn", FAT_OPEN_MODE_READ);
    if (!file) {
        printf("Failed to open /test.jsn\n");
        return;
    }

    // Allocate a buffer on the heap to hold the file content
    char* file_buffer = (char*)malloc(file->Size + 1);
    if (!file_buffer) {
        printf("malloc failed for file buffer!\n");
        FAT_Close(&g_Disk, file);
        getch();
        return;
    }

    // Read the entire file into the buffer
    uint32_t bytes_read = FAT_Read(&g_Disk, file, file->Size, file_buffer);
    if (bytes_read != file->Size) {
        printf("Error: Expected %u bytes, read %u\n", file->Size, bytes_read);
        free(file_buffer);
        FAT_Close(&g_Disk, file);
        return;
    }
    
    file_buffer[bytes_read] = '\0';
    FAT_Close(&g_Disk, file);
    printf("Read %u bytes from /test.jsn\n", bytes_read);

    // --- cJSON Parsing ---
    cJSON* root = cJSON_Parse(file_buffer);
    free(file_buffer); // Safe to free immediately after parsing

    if (!root) {
        printf("cJSON_Parse failed. Error near: %s\n", cJSON_GetErrorPtr());
        return;
    }

    printf("Successfully parsed JSON!\n");

    // --- Extracting values ---
    cJSON* message = cJSON_GetObjectItem(root, "message");
    if (cJSON_IsString(message)) {
        printf("  Message: %s\n", message->valuestring);
    }

    cJSON* kernel_name = cJSON_GetObjectItem(root, "kernel");
    if (cJSON_IsString(kernel_name)) {
        printf("  Kernel: %s\n", kernel_name->valuestring);
    }

    cJSON* year = cJSON_GetObjectItem(root, "year");
    if (cJSON_IsNumber(year)) {
        printf("  Year: %d\n", (int)year->valuedouble);
    }

    cJSON* is_awesome = cJSON_GetObjectItem(root, "is_awesome");
    if (cJSON_IsBool(is_awesome)) {
        printf("  Is Awesome? %s\n", cJSON_IsTrue(is_awesome) ? "true" : "false");
    }

    cJSON* features = cJSON_GetObjectItem(root, "features");
    if (cJSON_IsArray(features)) {
        printf("  Features:\n");
        cJSON* feature = NULL;
        cJSON_ArrayForEach(feature, features) {
            if (cJSON_IsString(feature) && (feature->valuestring != NULL)) {
                printf("    - %s\n", feature->valuestring);
            } else {
                printf("    - [Invalid Feature Element]\n");
            }
        }
    }

    // --- Cleanup ---
    cJSON_Delete(root); // This will test your free() implementation!
    printf("cJSON test complete. Memory freed.\n");
}