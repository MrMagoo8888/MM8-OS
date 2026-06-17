#include "libmm8.h"
#include "syscall.h"

void print(const char* str) {
    __asm__ volatile("int $0x80" : : "a"(SYS_PUTS), "b"(str));
}

void exit(int code) {
    __asm__ volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code));
    return; 
}

void* mm_malloc(uint32_t size) {
    uint32_t addr;
    __asm__ volatile("int $0x80" : "=a"(addr) : "a"(SYS_MALLOC), "b"(size));
    return (void*)addr;
}