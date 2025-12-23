#pragma once
#include <stdint.h>

void x86_64_outb(uint16_t port, uint8_t value);
uint8_t x86_64_inb(uint16_t port);
void x86_64_EnableInterrupts();
void x86_64_DisableInterrupts();

void x86_64_iowait();
void x86_64_Panic();

static inline void x86_64_insw(uint16_t port, void* buffer, uint32_t count)
{
    // Reads 'count' 16-bit values from I/O port 'port' into 'buffer'.
    // 'rep insw' is a string instruction that does this efficiently.
    // +D (buffer): edi register, read/write
    // +c (count): ecx register, read/write
    // d (port): edx register, input
    __asm__ __volatile__("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

static inline void x86_64_outsw(uint16_t port, const void* buffer, uint32_t count)
{
    // Writes 'count' 16-bit values from 'buffer' to I/O port 'port'.
    // 'rep outsw' is a string instruction that does this efficiently.
    // +S (buffer): esi register, read-only
    // +c (count): ecx register, read/write
    // d (port): edx register, input
    __asm__ __volatile__("rep outsw" : "+S"(buffer), "+c"(count) : "d"(port));
}