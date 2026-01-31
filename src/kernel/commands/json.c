#include "json.h"
#include "stdio.h"
#include "vfs.h"
#include "heap.h"
//#include "../src/kernel/libs/cjson/cJSON.h"
#include <libs/cJSON/cJSON.h>
#include <arch/i686/keyboard.h>

void handle_json_test() {
    printf("Running cJSON test...\n");

    VFS_File* file = VFS_Open("/test.json", "r");
    if (!file) {
        printf("Failed to open /test.json\n");
        return;
    }

    /* Read file into a dynamically grown buffer (don't rely on file->Size) */
    const size_t CHUNK = 512;
    size_t cap = CHUNK;
    size_t len = 0;
    char *buf = (char*)malloc(cap + 1);
    if (!buf) {
        printf("malloc failed for file buffer!\n");
        VFS_Close(file);
        return;
    }

    while (true) {
        uint32_t n = VFS_Read(file, CHUNK, buf + len);
        if (n == 0) break;
        len += n;
        if (len + 1 >= cap) {
            size_t newcap = cap * 2;
            char *nb = (char*)realloc(buf, newcap + 1);
            if (!nb) {
                printf("realloc failed\n");
                free(buf);
                VFS_Close(file);
                return;
            }
            buf = nb;
            cap = newcap;
        }
    }
    buf[len] = '\0';
    VFS_Close(file);

    printf("Read %u bytes from /test.json\n", (unsigned)len);

    // --- cJSON Parsing ---
    cJSON* root = cJSON_Parse(buf);

    free(buf);

    if (!root) {
        printf("cJSON_Parse failed. Error near: %s\n", cJSON_GetErrorPtr());
        return;
    }

    printf("Successfully parsed JSON!\n");

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
            if (cJSON_IsString(feature)) {
                printf("    - %s\n", feature->valuestring);
            }
        }
    }

    cJSON_Delete(root);
    printf("cJSON test complete. Memory freed.\n");
}