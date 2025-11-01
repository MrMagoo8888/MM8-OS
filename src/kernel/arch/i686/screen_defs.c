#include "screen_defs.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>   // <-- add

/* define text buffer pointer and cursor */
uint8_t *g_ScreenBuffer = (uint8_t*)0xB8000; /* adjust if different */
int g_ScreenX = 0;
int g_ScreenY = 0;

/* scrollback storage (sizes are compile-time constants via enum) */
uint16_t scrollback_buffer[SCROLLBACK_LINES][SCREEN_WIDTH];
int scrollback_start = 0;
int scrollback_count = 0;
int scrollback_view = 0;

/* make default color writable */
uint8_t DEFAULT_COLOR = 0x07; /* light gray on black by default */

/* live-screen backup buffer used when entering scrollback view */
uint8_t live_screen_backup[SCREEN_HEIGHT * SCREEN_WIDTH * 2];
bool in_scrollback_mode = false;

/* NOTE: getchr/getcolor/putcolor/refresh_screen_color may be implemented in stdio.c */
/* placeholder scrollback helpers (can be implemented here or left to stdio.c) */
void scrollback_lines(int lines) { /* optional: call into stdio's scrollback if needed */ }
void scrollforward_lines(int lines) { /* optional */ }