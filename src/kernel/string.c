#include "string.h"

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

unsigned int strlen(const char* str) {
    unsigned int len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dst, const char* src) {
    char* origDst = dst;
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
    return origDst;
}

int strncmp(const char* str1, const char* str2, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }
        if (str1[i] == '\0') {
            return 0; // Both strings are equal up to n or shorter
        }
    }
    return 0; // Strings are equal for n characters
}

const char* strchr(const char* str, int c) {
    while (*str != (char)c) {
        if (!*str++)
            return 0;
    }
    return str;
}

const char* strrchr(const char* str, int c) {
    const char* last = 0;
    do {
        if (*str == (char)c)
            last = str;
    } while (*str++);
    return last;
}