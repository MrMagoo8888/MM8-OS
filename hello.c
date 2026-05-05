#include <stdint.h>

// Syscall numbers matching syscall.h
#define SYS_EXIT 0
#define SYS_PUTS 1

void _start() {
    const char* msg = "Hello World! I am a real ELF binary loaded from disk.\n";

    // Invoke SYS_PUTS
    // EAX = syscall number, EBX = argument 1
    __asm__ volatile (
        "int $0x80"
        : : "a"(SYS_PUTS), "b"(msg)
    );

    // Invoke SYS_EXIT with exit code 0
    __asm__ volatile (
        "int $0x80"
        : : "a"(SYS_EXIT), "b"(0)
    );
}