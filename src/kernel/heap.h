#pragma once

#include <stdint.h>
#include <stddef.h>

void heap_initialize();
void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t new_size);
void heap_get_stats(size_t* total, size_t* used, size_t* free);