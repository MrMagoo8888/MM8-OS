#pragma once
#include <stdint.h>

void __attribute__((cdecl)) i686_outb(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) i686_inb(uint16_t port);
uint8_t __attribute__((cdecl)) i686_EnableInterrupts();
uint8_t __attribute__((cdecl)) i686_DisableInterrupts();

void __attribute__((cdecl)) i686_iowait();
void __attribute__((cdecl)) i686_Panic();

static inline void i686_insw(uint16_t port, void* buffer, uint32_t count)
{
    // Reads 'count' 16-bit values from I/O port 'port' into 'buffer'.
    // 'rep insw' is a string instruction that does this efficiently.
    // +D (buffer): edi register, read/write
    // +c (count): ecx register, read/write
    // d (port): edx register, input
    __asm__ __volatile__("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}