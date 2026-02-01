#pragma once

#include "memory.h"

int strcmp(const char* str1, const char* str2);
unsigned int strlen(const char* str);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strncmp(const char* str1, const char* str2, unsigned int n);
const char* strchr(const char* str, int c);
const char* strrchr(const char* str, int c);