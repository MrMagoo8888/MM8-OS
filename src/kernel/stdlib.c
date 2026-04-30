#include "stdlib.h"

/**
 * @brief Internal state for the pseudo-random number generator.
 */
static uint32_t g_RandState = 1;

/**
 * @brief Seeds the pseudo-random number generator.
 * @param seed The seed value.
 */
void srand(uint32_t seed) 
{
    g_RandState = seed;
}

/**
 * @brief Returns a pseudo-random integer in the range [0, RAND_MAX].
 */
int rand() 
{
    // Linear Congruential Generator (Numerical Recipes constants)
    g_RandState = g_RandState * 1664525 + 1013904223;
    return (int)(g_RandState & RAND_MAX);
}

/**
 * @brief Converts a string to an integer.
 */
int atoi(const char* str)
{
    int res = 0;
    int sign = 1;

    while (*str == ' ') str++;

    if (*str == '-') {
        sign = -1;
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }

    return res * sign;
}