#include <stddef.h>
#include "heap.h"
#include "string.h"

/* Minimal calloc using kernel malloc + zeroing */
void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *p = malloc(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}

/* Minimal strncpy (behaves like standard strncpy) */
char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n && src[i]) {
        dest[i] = src[i];
        ++i;
    }
    while (i < n) {
        dest[i++] = '\0';
    }
    return dest;
}

/* Simple selection-sort based qsort (O(n^2)) - small but safe for kernel use */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
    if (nmemb < 2 || size == 0 || !compar) return;
    char *b = (char*)base;
    /* temporary buffer for swaps */
    char *tmp = (char*)malloc(size);
    if (!tmp) return;

    for (size_t i = 0; i + 1 < nmemb; ++i) {
        size_t min = i;
        for (size_t j = i + 1; j < nmemb; ++j) {
            if (compar(b + j*size, b + min*size) < 0) {
                min = j;
            }
        }
        if (min != i) {
            memcpy(tmp, b + i*size, size);
            memcpy(b + i*size, b + min*size, size);
            memcpy(b + min*size, tmp, size);
        }
    }

    free(tmp);
}