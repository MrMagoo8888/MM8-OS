#include "hal.h"
// #include "arch/x86_64/gdt.h" // gdt_initialize() is called from main.c
#include "arch/x86_64/idt.h"
#include "arch/x86_64/isr.h"
#include "arch/x86_64/irq.h"

void HAL_Initialize()
{
    x86_64_IDT_Initialize();
    x86_64_ISR_Initialize();
    x86_64_IRQ_Initialize();
}

// Hardware Abstraction Layer