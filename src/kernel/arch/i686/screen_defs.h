#pragma once
#include <stdint.h>
#include <stdbool.h>   // <-- add

#define Color_Black         0x0
#define Color_Blue          0x1
#define Color_Green         0x2
#define Color_Cyan          0x3
#define Color_Red           0x4
#define Color_Magenta       0x5
#define Color_Brown         0x6
#define Color_LightGray     0x7
#define Color_DarkGray      0x8
#define Color_LightBlue     0x9
#define Color_LightGreen    0xA
#define Color_LightCyan     0xB
#define Color_LightRed      0xC
#define Color_LightMagenta  0xD
#define Color_Yellow        0xE
#define Color_White         0xF


//#define DEFAULT_COLOR 0x07

//extern unsigned char DEFAULT_COLOR;

/* compile-time constants for arrays (use enum to ensure integer constant) */
enum { SCROLLBACK_LINES = 200, SCREEN_WIDTH = 80, SCREEN_HEIGHT = 25 };

/* text buffer / cursor exported globals (defined in screen_defs.c) */
extern uint8_t *g_ScreenBuffer; /* VGA text buffer (0xB8000) */
extern int g_ScreenX;
extern int g_ScreenY;

/* scrollback storage (stores full 16-bit cells: char+attr) */
extern uint16_t scrollback_buffer[SCROLLBACK_LINES][SCREEN_WIDTH];
extern int scrollback_start;
extern int scrollback_count;
extern int scrollback_view;

/* default color (writable variable) */
extern uint8_t DEFAULT_COLOR;

/* live-screen backup & scrollback mode state (used by stdio.c) */
extern uint8_t live_screen_backup[SCREEN_HEIGHT * SCREEN_WIDTH * 2];
extern bool in_scrollback_mode;

/* helpers used by keyboard and commands */
uint16_t getcell(int x, int y);         /* returns full 16-bit cell */
char getchr(int x, int y);
uint8_t getcolor(int x, int y);
void putcell(int x, int y, uint16_t cell);
void putcolor(int x, int y, uint8_t color);
void refresh_screen_color(void);

/* scrollback helpers */
void scrollback_lines(int lines);   /* push top lines into scrollback and scroll up */
void scrollforward_lines(int lines);/* restore view from scrollback (page down) */

