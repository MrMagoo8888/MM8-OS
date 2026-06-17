/**
 * Minimal Hello World for MM8-OS
 * Linked to 0x1000000
 */

// Ensure these match the kernel's syscall.h
#include "libmm8.h"

void _start(int argc, char** argv) {
    print("Hello from a real ELF program!\n");
    
    if (argc > 1) {
        print("Argument 1: ");
        print(argv[1]);
        print("\n");
    }

    exit(0);
    return;
}