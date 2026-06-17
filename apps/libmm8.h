#pragma once
#include "stdint.h"

// Syscall wrappers
void print(const char* str);
void exit(int code);
void* mm_malloc(uint32_t size);