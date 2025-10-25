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