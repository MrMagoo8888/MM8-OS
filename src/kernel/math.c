#include "math.h"

// A very simple implementation of fabs
double fabs(double x) {
    if (x < 0) {
        return -x;
    }
    return x;
}

// STUB: A proper strtod is a lot of work. This is just to satisfy the linker
// for libraries like cJSON that might use it. It doesn't actually parse anything.
double strtod(const char* str, char** endptr) {
    if (endptr) {
        *endptr = (char*)str;
    }
    return 0.0;
}