#include "math.h"
#include "ctype.h"
#include "stdbool.h"

// A very simple implementation of fabs
double fabs(double x) {
    if (x < 0) {
        return -x;
    }
    return x;
}

double strtod(const char* str, char** endptr) {
    double res = 0.0;
    bool neg = false;
    const char* p = str;

    // Skip whitespace
    while (isspace(*p)) p++;

    // Handle sign
    if (*p == '-') {
        neg = true;
        p++;
    } else if (*p == '+') {
        p++;
    }

    const char* start = p;

    // Parse integer part
    while (isdigit(*p)) {
        res = res * 10.0 + (*p - '0');
        p++;
    }

    // Parse fractional part
    if (*p == '.') {
        p++;
        double div = 10.0;
        while (isdigit(*p)) {
            res += (*p - '0') / div;
            div *= 10.0;
            p++;
        }
    }

    if (neg) res = -res;

    if (endptr) {
        // If no digits were found, standard behavior returns original string
        *endptr = (char*)(p == start ? str : p);
    }

    return res;
}
