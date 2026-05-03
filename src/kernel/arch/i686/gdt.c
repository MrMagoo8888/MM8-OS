#include "gdt.h"
#include <stdint.h>
#include "memory.h"
#include "stdio.h"
// NOTES:

// LOOK AT BOOTLOADER ENTRY.ASM

typedef struct
{
    uint16_t LimitLow;                  // limit (bits 0-15)
    uint16_t BaseLow;                   // base (bits 0-15)
    uint8_t BaseMiddle;                 // base (bits 16-23)
    uint8_t Access;                     // access
    uint8_t FlagsLimitHi;               // limit (bits 16-19) | flags
    uint8_t BaseHigh;                   // base (bits 24-31)
} __attribute__((packed)) GDTEntry;

typedef struct
{
    uint16_t Limit;                     // sizeof(gdt) - 1
    GDTEntry* Ptr;                      // address of GDT
} __attribute__((packed)) GDTDescriptor;

typedef struct {
    uint32_t prev_tss; 
    uint32_t esp0;     // Kernel stack pointer
    uint32_t ss0;      // Kernel stack segment
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed)) TSSEntry;

static TSSEntry g_TSS;

void i686_TSS_SetStack(uint32_t kernelSS, uint32_t kernelESP) {
    g_TSS.ss0 = kernelSS;
    g_TSS.esp0 = kernelESP;
}

typedef enum
{
    GDT_ACCESS_CODE_READABLE                = 0x02,
    GDT_ACCESS_DATA_WRITEABLE               = 0x02,

    GDT_ACCESS_CODE_CONFORMING              = 0x04,
    GDT_ACCESS_DATA_DIRECTION_NORMAL        = 0x00,
    GDT_ACCESS_DATA_DIRECTION_DOWN          = 0x04,

    GDT_ACCESS_DATA_SEGMENT                 = 0x10,
    GDT_ACCESS_CODE_SEGMENT                 = 0x18,

    GDT_ACCESS_DESCRIPTOR_TSS               = 0x00,

    GDT_ACCESS_RING0                        = 0x00,
    GDT_ACCESS_RING1                        = 0x20,
    GDT_ACCESS_RING2                        = 0x40,
    GDT_ACCESS_RING3                        = 0x60,

    GDT_ACCESS_PRESENT                      = 0x80,

} GDT_ACCESS;

typedef enum 
{
    GDT_FLAG_64BIT                          = 0x20,
    GDT_FLAG_32BIT                          = 0x40,
    GDT_FLAG_16BIT                          = 0x00,

    GDT_FLAG_GRANULARITY_1B                 = 0x00,
    GDT_FLAG_GRANULARITY_4K                 = 0x80,
} GDT_FLAGS;

// Helper macros
#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)
#define GDT_BASE_MIDDLE(base)               ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HI(limit, flags)    (((limit >> 16) & 0xF) | (flags & 0xF0))
#define GDT_BASE_HIGH(base)                 ((base >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags) {                     \
    GDT_LIMIT_LOW(limit),                                           \
    GDT_BASE_LOW(base),                                             \
    GDT_BASE_MIDDLE(base),                                          \
    access,                                                         \
    GDT_FLAGS_LIMIT_HI(limit, flags),                               \
    GDT_BASE_HIGH(base)                                             \
}

GDTEntry g_GDT[6] = {
    // NULL descriptor
    GDT_ENTRY(0, 0, 0, 0),

    // Kernel 32-bit code segment
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // Kernel 32-bit data segment
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),
    
    // User 32-bit code segment
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // User 32-bit data segment
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // TSS segment
    GDT_ENTRY(0, 0, 0, 0) // Placeholder
};

GDTDescriptor g_GDTDescriptor = { sizeof(g_GDT) - 1, g_GDT};

void __attribute__((cdecl)) i686_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);

void i686_GDT_Initialize()
{
    // Initialize TSS
    memset(&g_TSS, 0, sizeof(g_TSS));
    g_TSS.ss0 = i686_GDT_DATA_SEGMENT;
    g_TSS.iomap_base = sizeof(g_TSS);

    uint32_t tss_base = (uint32_t)&g_TSS;
    uint32_t tss_limit = sizeof(g_TSS) - 1;

    // Set TSS gate manually because the macro doesn't handle the weird TSS access byte easily
    g_GDT[5] = (GDTEntry)GDT_ENTRY(tss_base, tss_limit, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR_TSS | 0x09, 0);

    i686_GDT_Load(&g_GDTDescriptor, i686_GDT_CODE_SEGMENT, i686_GDT_DATA_SEGMENT);
    
    // Load TSS register
    __asm__ volatile("ltr %%ax" : : "a" (i686_GDT_TSS_SEGMENT));
}