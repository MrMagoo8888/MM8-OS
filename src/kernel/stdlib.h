#pragma once

#include "heap.h"

// This file provides standard library function declarations for our kernel.
// When a library includes <stdlib.h>, the compiler will find this file.

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
void gets(char* buffer, int size);