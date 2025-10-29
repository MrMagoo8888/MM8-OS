#include "math.h"
#include "ctype.h"
#include "ctype.h"

double fabs(double x) {
    return (x < 0) ? -x : x;
}

// A very simple implementation of strtod.
// It does not handle exponents (e.g., "1.23e4").
double strtod(const char* str, char** endptr) {
    double res = 0.0;
    double sign = 1.0;
    int i = 0;

    // Handle sign
    if (str[i] == '-') {
        sign = -1.0;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    // Handle integer part
    while (str[i] && isdigit(str[i])) {
        res = res * 10.0 + (str[i] - '0');
        i++;
    }

    // Handle fractional part
    if (str[i] == '.') {
        i++;
        double frac = 0.1;
        while (str[i] && isdigit(str[i])) {
            res += (str[i] - '0') * frac;
            frac *= 0.1;
            i++;
        }
    }

    if (endptr) *endptr = (char*)&str[i];
    return res * sign;
}