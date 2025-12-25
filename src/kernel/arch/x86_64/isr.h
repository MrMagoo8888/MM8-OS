#pragma once
#include "stdint.h"

typedef struct 
{
    // Pushed by our common stub (x86_64 doesn't have pusha)
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    
    // Pushed by stub or CPU
    uint64_t interrupt, error;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) Registers;

typedef void (*ISRHandler)(Registers* regs);

void x86_64_ISR_Initialize();
void x86_64_ISR_RegisterHandler(int interrupt, ISRHandler handler);