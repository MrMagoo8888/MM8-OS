#include <stdio.h>
#include <arch/i686/io.h>
#include <stdint.h>
#include "stdio.h"
#include "stdbool.h"
#include <stdarg.h>
#include "memory.h"
#include "vbe.h"
#include "graphics.h"
#include "font.h"
#include "heap.h"

// VGA Color Palette (0-15) mapped to 32-bit RGB
static const uint32_t vga_colors[16] = {
    0x00000000, // 0: Black
    0x000000AA, // 1: Blue
    0x0000AA00, // 2: Green
    0x0000AAAA, // 3: Cyan
    0x00AA0000, // 4: Red
    0x00AA00AA, // 5: Magenta
    0x00AA5500, // 6: Brown
    0x00AAAAAA, // 7: Light Gray
    0x00555555, // 8: Dark Gray
    0x005555FF, // 9: Light Blue
    0x0055FF55, // 10: Light Green
    0x0055FFFF, // 11: Light Cyan
    0x00FF5555, // 12: Light Red
    0x00FF55FF, // 13: Light Magenta
    0x00FFFF55, // 14: Yellow
    0x00FFFFFF  // 15: White
};

// Shadow buffer for text (since we can't read back from VBE easily)
// Assuming 80x25 standard text resolution for logic
// static uint8_t g_ShadowBuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 2];
static uint8_t* g_ShadowBuffer = NULL;
uint8_t* g_ScreenBuffer = NULL;

int g_ScreenX = 0, g_ScreenY = 0;
int g_ConsoleWidth = 80;
int g_ConsoleHeight = 25;
int g_FontScale = 1; // Scale 1x by default

// --- Scrollback Buffer ---
// char scrollback_buffer[SCROLLBACK_LINES][SCREEN_WIDTH];
char* scrollback_buffer = NULL; // Flat buffer: SCROLLBACK_LINES * g_ConsoleWidth
int scrollback_start = 0;
int scrollback_count = 0;
int scrollback_view = 0;

// --- Live Screen Backup ---
// static uint8_t live_screen_backup[SCREEN_HEIGHT * SCREEN_WIDTH * 2];
static uint8_t* live_screen_backup = NULL;
static bool in_scrollback_mode = false;

void console_initialize() {
    if (g_vbe_screen) {
        g_ConsoleWidth = g_vbe_screen->width / (8 * g_FontScale);
        g_ConsoleHeight = g_vbe_screen->height / (8 * g_FontScale);
    }

    // Allocate buffers based on dynamic size
    g_ShadowBuffer = (uint8_t*)malloc(g_ConsoleWidth * g_ConsoleHeight * 2);
    g_ScreenBuffer = g_ShadowBuffer;
    
    live_screen_backup = (uint8_t*)malloc(g_ConsoleWidth * g_ConsoleHeight * 2);
    scrollback_buffer = (char*)malloc(SCROLLBACK_LINES * g_ConsoleWidth);

    // Clear buffers
    if (g_ShadowBuffer) memset(g_ShadowBuffer, 0, g_ConsoleWidth * g_ConsoleHeight * 2);
    if (scrollback_buffer) memset(scrollback_buffer, 0, SCROLLBACK_LINES * g_ConsoleWidth);
}

// Helper to draw a character to the VBE screen
static void draw_char_at(int x, int y, char c, uint8_t color) {
    if (!g_vbe_screen) return;

    uint32_t fg = vga_colors[color & 0x0F];
    uint32_t bg = vga_colors[(color >> 4) & 0x0F];

    // Get font data (offset by 32 because our font starts at space)
    const uint8_t* glyph = (c >= 32 && c <= 127) ? font8x8_basic[c - 32] : font8x8_basic[0];

    int screen_x = x * 8 * g_FontScale;
    int screen_y = y * 8 * g_FontScale;

    // Draw 8x8 pixels
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // Check if the bit is set in the font bitmap
            // Bit 0 is the rightmost pixel, Bit 7 is leftmost
            bool pixel_on = (glyph[row] >> (7 - col)) & 1;
            uint32_t draw_color = pixel_on ? fg : bg;
            
            // Draw scaled pixel
            for (int sy = 0; sy < g_FontScale; sy++) {
                for (int sx = 0; sx < g_FontScale; sx++) {
                    draw_pixel(screen_x + col * g_FontScale + sx, 
                               screen_y + row * g_FontScale + sy, 
                               draw_color);
                }
            }
        }
    }
}

void putchr(int x, int y, char c)
{
    if (!g_ScreenBuffer) return;
    // Update shadow buffer
    g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)] = c;
    
    // Draw to VBE
    draw_char_at(x, y, c, getcolor(x, y));
}

void putcolor(int x, int y, uint8_t color)
{
    if (!g_ScreenBuffer) return;
    // Update shadow buffer
    g_ScreenBuffer[2 * (y * g_ConsoleWidth + x) + 1] = color;
    
    // Draw to VBE
    draw_char_at(x, y, getchr(x, y), color);
}

char getchr(int x, int y)
{
    if (!g_ScreenBuffer) return 0;
    return g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)];
}

uint8_t getcolor(int x, int y)
{
    if (!g_ScreenBuffer) return 0;
    return g_ScreenBuffer[2 * (y * g_ConsoleWidth + x) + 1];
}

void setcursor(int x, int y)
{
    // Hardware cursor (VGA ports) doesn't work in VBE mode.
    // We would need to implement a software cursor (e.g., drawing an underscore).
    // For now, we just disable the port writes to avoid issues.
    
    // int pos = y * SCREEN_WIDTH + x;
    // i686_outb(0x3D4, 0x0F);
    // i686_outb(0x3D5, (uint8_t)(pos & 0xFF));
    // i686_outb(0x3D4, 0x0E);
    // i686_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clrscr()
{
    if (!g_ScreenBuffer) return;
    // Clear the shadow buffer
    for (int y = 0; y < g_ConsoleHeight; y++)
        for (int x = 0; x < g_ConsoleWidth; x++)
        {
            g_ScreenBuffer[2 * (y * g_ConsoleWidth + x)] = '\0';
            g_ScreenBuffer[2 * (y * g_ConsoleWidth + x) + 1] = DEFAULT_COLOR;
        }

    g_ScreenX = 0;
    g_ScreenY = 0;
    
    // Clear the VBE screen (fill with black)
    if (g_vbe_screen) {
        // Calculate total bytes: pitch * height
        // Note: This assumes the screen is contiguous, which is usually true for VBE.
        // A safer way is to clear line by line if pitch != width * bpp
        memset((void*)g_vbe_screen->physical_buffer, 0, g_vbe_screen->height * g_vbe_screen->pitch);
    }
}

void scrollback(int lines)
{
    if (!scrollback_buffer) return;
    // Save scrolled-off lines to the scrollback buffer
    for (int i = 0; i < lines; i++) {
        // Save the characters of the top line (y=0)
        int row_idx = (scrollback_start + scrollback_count) % SCROLLBACK_LINES;
        for (int x = 0; x < g_ConsoleWidth; x++) {
            scrollback_buffer[row_idx * g_ConsoleWidth + x] = getchr(x, 0);
        }

        if (scrollback_count < SCROLLBACK_LINES) {
            scrollback_count++;
        } else {
            scrollback_start = (scrollback_start + 1) % SCROLLBACK_LINES;
        }
    }

    // Move all lines up
    for (int y = lines; y < g_ConsoleHeight; y++) {
        for (int x = 0; x < g_ConsoleWidth; x++) {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }
    }

    // Clear the bottom lines
    for (int y = g_ConsoleHeight - lines; y < g_ConsoleHeight; y++) {
        for (int x = 0; x < g_ConsoleWidth; x++) {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }

    g_ScreenY -= lines;
}

void refresh_screen_color()
{
    for (int y = 0; y < g_ConsoleHeight; y++) {
        for (int x = 0; x < g_ConsoleWidth; x++) {
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
}

static void redraw_from_scrollback() {
    int top_history_line = scrollback_count - scrollback_view;
    for (int y = 0; y < g_ConsoleHeight; y++) {
        int history_line_index = top_history_line - (g_ConsoleHeight - 1 - y);
        if (history_line_index >= 0 && history_line_index < scrollback_count) {
            int buffer_idx = (scrollback_start + history_line_index) % SCROLLBACK_LINES;
            for (int x = 0; x < g_ConsoleWidth; x++) {
                putchr(x, y, scrollback_buffer[buffer_idx * g_ConsoleWidth + x]);
                putcolor(x, y, DEFAULT_COLOR);
            }
        } else {
            // Clear lines that are beyond the history
            for (int x = 0; x < g_ConsoleWidth; x++) {
                putchr(x, y, ' ');
                putcolor(x, y, DEFAULT_COLOR);
            }
        }
    }
    setcursor(0, g_ConsoleHeight - 1);
}

void view_scrollback_up() {
    if (scrollback_count == 0) return;
    if (!in_scrollback_mode) {
        memcpy(live_screen_backup, g_ScreenBuffer, g_ConsoleWidth * g_ConsoleHeight * 2);
        in_scrollback_mode = true;
    }
    scrollback_view++;
    if (scrollback_view > scrollback_count) {
        scrollback_view = scrollback_count;
    }
    redraw_from_scrollback();
}

void view_scrollback_down() {
    if (!in_scrollback_mode) return;
    scrollback_view--;
    if (scrollback_view <= 0) {
        scrollback_view = 0;
        in_scrollback_mode = false;
        memcpy(g_ScreenBuffer, live_screen_backup, g_ConsoleWidth * g_ConsoleHeight * 2);
        setcursor(g_ScreenX, g_ScreenY);
        return;
    }
    redraw_from_scrollback();
}

void putc(char c)
{
    switch (c)
    {
        case '\n':
            g_ScreenX = 0;
            g_ScreenY++;
            break;

        case '\b':
            if (g_ScreenX > 0) {
                g_ScreenX--;
                putchr(g_ScreenX, g_ScreenY, ' ');
            } else if (g_ScreenY > 0) {
                g_ScreenY--;
                g_ScreenX = g_ConsoleWidth - 1;
                putchr(g_ScreenX, g_ScreenY, ' ');
            }
            break;
    
        case '\t':
            for (int i = 0; i < 4 - (g_ScreenX % 4); i++)
                putc(' ');
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        default:
            putchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;
    }

    if (g_ScreenX >= g_ConsoleWidth)
    {
        g_ScreenY++;
        g_ScreenX = 0;
    }
    if (g_ScreenY >= g_ConsoleHeight)
        scrollback(1);

    setcursor(g_ScreenX, g_ScreenY);
}

void puts(const char* str)
{
    while(*str)
    {
        putc(*str);
        str++;
    }
}

const char g_HexChars[] = "0123456789abcdef";

void printf_unsigned(unsigned long long number, int radix)
{
    char buffer[32];
    int pos = 0;

    // convert number to ASCII
    do 
    {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0)
        putc(buffer[pos]);
}

void printf_signed(long long number, int radix)
{
    if (number < 0)
    {
        putc('-');
        printf_unsigned(-number, radix);
    }
    else printf_unsigned(number, radix);
}

#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4

void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;
    bool number = false;

    while (*fmt)
    {
        switch (state)
        {
            case PRINTF_STATE_NORMAL:
                switch (*fmt)
                {
                    case '%':   state = PRINTF_STATE_LENGTH;
                                break;
                    default:    putc(*fmt);
                                break;
                }
                break;

            case PRINTF_STATE_LENGTH:
                switch (*fmt)
                {
                    case 'h':   length = PRINTF_LENGTH_SHORT;
                                state = PRINTF_STATE_LENGTH_SHORT;
                                break;
                    case 'l':   length = PRINTF_LENGTH_LONG;
                                state = PRINTF_STATE_LENGTH_LONG;
                                break;
                    default:    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h')
                {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l')
                {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt)
                {
                    case 'c':   putc((char)va_arg(args, int));
                                break;

                    case 's':   
                                puts(va_arg(args, const char*));
                                break;

                    case '%':   putc('%');
                                break;

                    case 'd':
                    case 'i':   radix = 10; sign = true; number = true;
                                break;

                    case 'u':   radix = 10; sign = false; number = true;
                                break;

                    case 'X':
                    case 'x':
                    case 'p':   radix = 16; sign = false; number = true;
                                break;

                    case 'o':   radix = 8; sign = false; number = true;
                                break;

                    // ignore invalid spec
                    default:    break;
                }

                if (number)
                {
                    if (sign)
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:     printf_signed(va_arg(args, int), radix);
                                                        break;

                        case PRINTF_LENGTH_LONG:        printf_signed(va_arg(args, long), radix);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   printf_signed(va_arg(args, long long), radix);
                                                        break;
                        }
                    }
                    else
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:     printf_unsigned(va_arg(args, unsigned int), radix);
                                                        break;
                                                        
                        case PRINTF_LENGTH_LONG:        printf_unsigned(va_arg(args, unsigned  long), radix);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   printf_unsigned(va_arg(args, unsigned  long long), radix);
                                                        break;
                        }
                    }
                }

                // reset state
                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = false;
                number = false;
                break;
        }

        fmt++;
    }

    va_end(args);
}

void print_buffer(const char* msg, const void* buffer, uint32_t count)
{
    const uint8_t* u8Buffer = (const uint8_t*)buffer;
    
    puts(msg);
    for (uint16_t i = 0; i < count; i++)
    {
        putc(g_HexChars[u8Buffer[i] >> 4]);
        putc(g_HexChars[u8Buffer[i] & 0xF]);
    }
    puts("\n");
}

// STUB: A proper sprintf is a lot of work. This is just to satisfy the linker.
int sprintf(char* str, const char* format, ...)
{
    // For now, just create an empty string. This will cause issues for cJSON's
    // number printing, but it will allow the kernel to link.
    str[0] = '\0';
    return 0;
}

// STUB: A proper sscanf is a lot of work.
int sscanf(const char* str, const char* format, ...) {
    return 0; // Pretend we parsed nothing.
}