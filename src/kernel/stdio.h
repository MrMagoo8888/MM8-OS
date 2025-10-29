#pragma once
#include "stdint.h"
#include "arch/i686/screen_defs.h"

void clrscr();
void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);
char getchr(int x, int y);
void putcolor(int x, int y, uint8_t color);
void putchr(int x, int y, char c);
uint8_t getcolor(int x, int y);
void setcursor(int x, int y);
void scrollback(int lines);
void scrollforward(int lines);

void view_scrollback_up();
void view_scrollback_down();

int sprintf(char* str, const char* format, ...);
int sscanf(const char* str, const char* format, ...);