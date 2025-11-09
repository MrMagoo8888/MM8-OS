#pragma once

#include <stdint.h>
#include <stddef.h>
#include "vbe.h"

void heap_initialize(vbe_screen_t* vbe_info);
void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t new_size);