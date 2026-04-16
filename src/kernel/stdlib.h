#pragma once

#include "stdint.h"
#include "stddef.h"

/**
 * @brief The maximum value returned by rand().
 */
#define RAND_MAX 0x7FFFFFFF

void srand(uint32_t seed);
int rand();

void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t new_size);