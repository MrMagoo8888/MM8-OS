#include "math.h"

// A very simple implementation of fabs
double fabs(double x) {
    if (x < 0) {
        return -x;
    }
    return x;
}

// A simplified strtod implementation for cJSON.
// This handles basic integers and floating-point numbers without full error checking or scientific notation.
double strtod(const char* str, char** endptr) {
    double res = 0.0;
    double sign = 1.0;
    const char* p = str;

    // Skip leading whitespace
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }

    // Handle sign
    if (*p == '-') {
        sign = -1.0;
        p++;
    } else if (*p == '+') {
        p++;
    }

    // Handle integer part
    while (*p >= '0' && *p <= '9') {
        res = res * 10.0 + (*p - '0');
        p++;
    }

    // Handle fractional part
    if (*p == '.') {
        p++;
        double f = 0.0;
        double div = 1.0;
        while (*p >= '0' && *p <= '9') {
            f = f * 10.0 + (*p - '0');
            div *= 10.0;
            p++;
        }
        res += f / div;
    }

    if (endptr) {
        *endptr = (char*)p;
    }
    return res * sign;
}