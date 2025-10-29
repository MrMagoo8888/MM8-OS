#include "json.h"
#include "stdio.h"
#include "fat.h"
#include "heap.h"
//#include "../src/kernel/libs/cjson/cJSON.h"
#include <libs/cJSON/cJSON.h>

void handle_json_test() {
    printf("Running cJSON test...\n");

    FAT_File* file = FAT_Open(&g_Disk, "/test.json", FAT_OPEN_MODE_READ);
    if (!file) {
        printf("Failed to open /test.json\n");
        return;
    }

    // Allocate a buffer on the heap to hold the file content
    char* file_buffer = (char*)malloc(file->Size + 1);
    if (!file_buffer) {
        printf("malloc failed for file buffer!\n");
        FAT_Close(&g_Disk, file);
        return;
    }

    // Read the entire file into the buffer
    uint32_t bytes_read = FAT_Read(&g_Disk, file, file->Size, file_buffer);
    file_buffer[bytes_read] = '\0'; // Null-terminate the string

    FAT_Close(&g_Disk, file);

    printf("Read %u bytes from /test.json\n", bytes_read);

    // --- cJSON Parsing ---
    cJSON* root = cJSON_Parse(file_buffer);

    // Free the file buffer now that cJSON has parsed it
    free(file_buffer);

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
            printf("    - %s\n", feature->valuestring);
        }
    }

    // --- Cleanup ---
    cJSON_Delete(root); // This will test your free() implementation!
    printf("cJSON test complete. Memory freed.\n");
}