#include "isr.h"
#include "idt.h"
#include "gdt.h"
#include "io.h"
#include <stdio.h>
#include <stddef.h>

ISRHandler g_ISRHandlers[256];

static const char* const g_Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};

void i686_ISR_InitializeGates();
void __attribute__((cdecl)) i686_ISR128();

static void i686_Syscall_Handler(Registers* regs)
{
    // Use EAX as the syscall number
    switch (regs->eax)
    {
        case 1: // Syscall 1: putc(char c)
            putc((char)regs->ebx);
            break;
        case 2: // Syscall 2: puts(const char* s)
            puts((const char*)regs->ebx);
            break;
        default:
            printf("Syscall: Unknown syscall %d\n", regs->eax);
            break;
    }
}

void i686_ISR_Initialize()
{
    i686_ISR_InitializeGates();
    for (int i = 0; i < 256; i++)
        i686_IDT_EnableGate(i);
    
    // Override 0x80 gate to allow Ring 3 (User Mode) to trigger it
    i686_IDT_SetGate(0x80, i686_ISR128, i686_GDT_CODE_SEGMENT, IDT_FLAG_RING3 | IDT_FLAG_GATE_32BIT_INT);
    i686_ISR_RegisterHandler(0x80, i686_Syscall_Handler);
}

void __attribute__((cdecl)) i686_ISR_Handler(Registers* regs)
{
    if (g_ISRHandlers[regs->interrupt] != NULL)
        g_ISRHandlers[regs->interrupt](regs);

    else if (regs->interrupt >= 32)
        printf("Unhandled interrupt %d!\n", regs->interrupt);

    else 
    {
        printf("Unhandled exception %d %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
        
        printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

        printf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
               regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds, regs->ss);

        printf("  interrupt=%x  errorcode=%x\n", regs->interrupt, regs->error);

        printf("KERNEL PANIC!\n");
        i686_Panic();
    }
}

void i686_ISR_RegisterHandler(int interrupt, ISRHandler handler)
{
    g_ISRHandlers[interrupt] = handler;
    i686_IDT_EnableGate(interrupt);
}