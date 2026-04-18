#pragma once
#include <stdint.h>
#include <stddef.h>

void* memcpy(void* dst, const void* src, size_t num);
void* memset(void* ptr, int value, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
void* memmove(void* dst, const void* src, size_t num);

// Allocation functions (Defined in heap.c)
void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t new_size);

void* malloc_aligned(size_t size, size_t alignment);
void free_aligned(void* ptr);