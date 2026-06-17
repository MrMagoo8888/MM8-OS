#include "syscall.h"
#include <arch/i686/isr.h>
#include "stdio.h"
#include "memory.h"
#include "heap.h"

static void syscall_handler(Registers* regs) {
    // Syscall number is in EAX
    // Arguments are in EBX, ECX, EDX, ESI, EDI
    uint32_t syscall_num = regs->eax;

    switch (syscall_num) {
        case SYS_PUTS:
            // EBX contains the pointer to the string HELP probs validate lol
            puts((const char*)regs->ebx);
            break;

            // allcate memory
        case SYS_MALLOC:
            printf("SYSCALL: Malloc %u bytes\n", regs->ebx);
            regs->eax = (uint32_t)malloc(regs->ebx);
            break;
            
        case SYS_FREE:
            free((void*)regs->ebx);
            break;

        case SYS_EXIT:
            printf("\nProcess exited with code %d\n", regs->ebx);
            // In multi-tasking, you would terminate the task here.
            break;

        default:
            printf("Unknown syscall: %d\n", syscall_num);
            break;
    }
}

void syscall_initialize() {
    // Register handler for interrupt 0x80 (128 decimal)
    i686_ISR_RegisterHandler(0x80, syscall_handler);
}