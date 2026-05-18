/**
 * Minimal Hello World for MM8-OS
 * Linked to 0x1000000
 */

// Define syscall numbers based on the order in your syscall_handler switch
#define SYS_PUTS  0
#define SYS_EXIT  3

void print(const char* str) {
    // EAX = syscall number, EBX = argument 1
    asm volatile("int $0x80" : : "a"(SYS_PUTS), "b"(str));
}

void exit(int code) {
    asm volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code));
}

void _start() {
    print("Hello from a real ELF program!\n");
    print("MM8-OS Syscall interface is working.\n");
    
    exit(0);
}