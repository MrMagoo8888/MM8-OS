#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include <arch/i686/keyboard.h> // This include is already present, but good to confirm
#include "fat.h"
#include <arch/i686/keyboard.h>
#include "string.h"
#include "heap.h"
#include <commands/command.h>
#include <apps/editor/editor.h>
#include "globals.h"

/* --- ADDED: framebuffer + serial hex helpers for early diagnostics --- */
#include "framebuffer.h"
#include "framebuffer_text.h"
#include "fonts/font.h"

static void serial_puthex8(uint8_t v) {
    const char *h = "0123456789ABCDEF";
    char s[3] = { h[(v >> 4) & 0xF], h[v & 0xF], 0 };
    serial_puts(s);
}
static void serial_puthex32(uint32_t v) {
    const char *h = "0123456789ABCDEF";
    char s[9];
    for (int i = 0; i < 8; ++i) s[7 - i] = h[(v >> (i * 4)) & 0xF];
    s[8] = '\0';
    serial_puts("0x"); serial_puts(s);
}
/* --- END ADDED --- */

// --- ADDED: very small COM1 debug helpers (early boot diagnostics) ---
static inline void __serial_outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline unsigned char __serial_inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void serial_init(void) {
    /* COM1=0x3F8 init: disable IRQ, set baud 38400, 8N1, enable FIFO */
    __serial_outb(0x3F8 + 1, 0x00); // disable all interrupts
    __serial_outb(0x3F8 + 3, 0x80); // enable DLAB
    __serial_outb(0x3F8 + 0, 0x03); // divisor low 3 (38400)
    __serial_outb(0x3F8 + 1, 0x00); // divisor high
    __serial_outb(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
    __serial_outb(0x3F8 + 2, 0xC7); // enable FIFO, clear them, with 14-byte threshold
    __serial_outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static void serial_putc(char c) {
    while (( __serial_inb(0x3F8 + 5) & 0x20) == 0) { /* wait for transmitter empty */ }
    __serial_outb(0x3F8, (unsigned char)c);
}

static void serial_puts(const char *s) {
    while (*s) serial_putc(*s++);
}
/* tiny hex printer to serial */
static void serial_puthex32(uint32_t v) {
    const char *hex = "0123456789ABCDEF";
    char buf[9];
    for (int i = 0; i < 8; ++i) {
        buf[7-i] = hex[(v >> (i*4)) & 0xF];
    }
    buf[8] = '\0';
    serial_puts("0x"); serial_puts(buf);
}
// --- END ADDED ---

/* forward declaration so we can call test_framebuffer() before its definition */
static void test_framebuffer(void);

DISK g_Disk;

extern uint8_t __bss_start;
extern uint8_t __end;

void timer(Registers* regs)
{
    // This function is called by the system timer (IRQ 0).
    // For now, it does nothing but is required to acknowledge the interrupt.
}

#define HISTORY_SIZE 10 // Define the size of the command history
static char g_CommandHistory[HISTORY_SIZE][256];
static int g_HistoryCount = 0;
static int g_HistoryIndex = 0;

void add_to_history(const char* command) {
    if (command[0] == '\0') return; // Don't save empty commands
    // Don't save if it's the same as the last command
    if (g_HistoryCount > 0 && strcmp(command, g_CommandHistory[(g_HistoryIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE]) == 0) {
        return;
    }
    strcpy(g_CommandHistory[g_HistoryIndex], command);
    g_HistoryIndex = (g_HistoryIndex + 1) % HISTORY_SIZE;
    if (g_HistoryCount < HISTORY_SIZE) {
        g_HistoryCount++;
    }
}

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    /* very early: initialize COM1 and emit a marker so we can see boot progress */
    serial_init();
    serial_puts("EARLY: serial initialized\n");

    /* --- ADDED: framebuffer diagnostics (prints fb_info and a 64-byte LFB dump) --- */
    FramebufferInfo *dbg_f = fb_info();
    if (dbg_f) {
        serial_puts("FB: phys=");
        serial_puthex32((uint32_t)dbg_f->framebuffer_phys);
        serial_puts(" w=");
        serial_puthex32((uint32_t)dbg_f->width);
        serial_puts(" h=");
        serial_puthex32((uint32_t)dbg_f->height);
        serial_puts(" pitch=");
        serial_puthex32((uint32_t)dbg_f->pitch);
        serial_puts(" bpp=");
        serial_puthex32((uint32_t)dbg_f->bpp);
        serial_puts("\n");

        if (dbg_f->framebuffer_phys) {
            serial_puts("FB DUMP:");
            uint8_t *p = (uint8_t*)(uintptr_t)dbg_f->framebuffer_phys;
            for (int i = 0; i < 64; ++i) {
                if ((i & 15) == 0) {
                    serial_puts("\n");
                    serial_puthex32((uint32_t)i);
                    serial_puts(": ");
                }
                serial_puthex8(p[i]);
                serial_puts(" ");
            }
            serial_puts("\n");
        } else {
            serial_puts("FB physical addr == 0\n");
        }
    } else {
        serial_puts("fb_info() returned NULL\n");
    }
    /* --- END ADDED --- */

    memset(&__bss_start, 0, (&__end) - (&__bss_start));
    
    HAL_Initialize();

    heap_initialize();

    /* debug: print framebuffer info so we can see whether LFB was set */
    printf("FB: phys=0x%08x w=%u h=%u pitch=%u bpp=%u\n",
           dbg_f->framebuffer_phys,
           (unsigned)dbg_f->width,
           (unsigned)dbg_f->height,
           (unsigned)dbg_f->pitch,
           (unsigned)dbg_f->bpp);

    test_framebuffer();
    
    //clrscr();
    
    mm8Splash();

    FramebufferInfo *f = fb_info();
    if (f->framebuffer_phys) {
        fb_clear(0x000000); // black
        fb_draw_text_centered(1, "MM8-OS (graphics mode)", 0x00FFFFFF);
        fb_draw_text_centered(3, "Hello from framebuffer!", 0x00FFDDFF);
    } else {
        // fallback: existing text-mode console init
    }

    printf("\n\nType 'help' for a list of commands.\n\n");

    // Initialize disk and FAT filesystem
    // We are booting from floppy, but we want to use the first hard disk (0x80) for our root filesystem.
    if (!DISK_Initialize(&g_Disk, 0x80)) {
        printf("Hard disk initialization failed.\n");
    } else if (!FAT_Initialize(&g_Disk)) {
        printf("FAT initialization failed on hard disk.\n");
    }

    i686_IRQ_RegisterHandler(0, timer);
    i686_Keyboard_Initialize(g_CommandHistory, &g_HistoryCount, &g_HistoryIndex, HISTORY_SIZE);

    // Enable interrupts now that all handlers are set up
    i686_EnableInterrupts();

    char input_buffer[256];

    while (1) {
        printf("> ");


        gets(input_buffer, sizeof(input_buffer));

        add_to_history(input_buffer);


        command_dispatch(input_buffer); // invokes commands like help, cls, echo, read, edit
    
    }

    // This part is now unreachable
    for (;;);
}

/* Small framebuffer smoke-test: clear, draw centered text and a filled box. */
static void test_framebuffer(void) {
    FramebufferInfo *f = fb_info();
    if (!f->framebuffer_phys) return;

    /* clear to black */
    fb_clear(0x000000);

    /* draw test strings */
    fb_draw_text_centered(1, "MM8-OS framebuffer test", 0x00FFFFFF);
    fb_draw_text_centered(3, "Renderer OK - boot verified", 0x00FFFF88);

    /* filled 8x8 block glyph (all pixels on) */
    const uint8_t filled_glyph[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    /* draw a colored rectangle by tiling filled glyphs (in character cells) */
    int start_cx = 4;
    int start_cy = 6;
    int w_cells = 20;
    int h_cells = 6;
    uint32_t color = 0x0000FF00; /* green (0x00RRGGBB) */

    for (int cy = 0; cy < h_cells; ++cy) {
        for (int cx = 0; cx < w_cells; ++cx) {
            int px = (start_cx + cx) * 8;
            int py = (start_cy + cy) * 8;
            fb_draw_glyph(px, py, color, filled_glyph);
        }
    }
}