#include "idt.h"
#include <stdint.h>
#include "../../util/binary.h"

typedef struct
{
    uint16_t BaseLow;
    uint16_t SegmentSelector;
    uint8_t  IST;       // Interrupt Stack Table offset
    uint8_t Flags;
    uint16_t BaseMid;
    uint32_t BaseHigh;
    uint32_t Reserved;
} __attribute__((packed)) IDTEntry;

typedef struct
{
    uint16_t Limit;
    uint64_t Ptr;
} __attribute__((packed)) IDTDescriptor;


IDTEntry g_IDT[256];

IDTDescriptor g_IDTDescriptor = { sizeof(g_IDT) - 1, (uint64_t)g_IDT };

void x86_64_IDT_Load(IDTDescriptor* idtDescriptor);

void x86_64_IDT_SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags)
{
    uint64_t addr = (uint64_t)base;
    g_IDT[interrupt].BaseLow = addr & 0xFFFF;
    g_IDT[interrupt].SegmentSelector = segmentDescriptor;
    g_IDT[interrupt].IST = 0;
    g_IDT[interrupt].Flags = flags;
    g_IDT[interrupt].BaseMid = (addr >> 16) & 0xFFFF;
    g_IDT[interrupt].BaseHigh = (addr >> 32) & 0xFFFFFFFF;
    g_IDT[interrupt].Reserved = 0;
}

void x86_64_IDT_EnableGate(int interrupt)
{
    FLAG_SET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}

void x86_64_IDT_DisableGate(int interrupt)
{
    FLAG_UNSET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}

void x86_64_IDT_Initialize()
{
    x86_64_IDT_Load(&g_IDTDescriptor);
}