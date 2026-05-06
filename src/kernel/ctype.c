#include "ctype.h"

int toupper(int c) {
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';
    return c;
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    return c;
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isprint(int c) {
    return (c >= 0x20 && c <= 0x7E);
}

int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}