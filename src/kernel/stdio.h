#pragma once
#include <stdint.h>

/* Basic console helpers used by multiple modules.
   Implementations live in stdio.c / screen_defs.c */
void setcursor(int x, int y);
void putc(char c);               /* put char at current cursor and advance */
void putchr(int x, int y, char c); /* write char to given position (no cursor move) */

char getchr(int x, int y);
uint8_t getcolor(int x, int y);
void putcolor(int x, int y, uint8_t color);
void refresh_screen_color(void);

/* scrollback helpers */
void scrollback_lines(int lines);
void scrollforward_lines(int lines);

void clrscr();
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);
void view_scrollback_up();
void view_scrollback_down();

int sprintf(char* str, const char* format, ...);
int sscanf(const char* str, const char* format, ...);