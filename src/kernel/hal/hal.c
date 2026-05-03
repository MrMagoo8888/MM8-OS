#include "hal.h"
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/isr.h>
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include <arch/i686/paging.h>

void i686_PIT_Initialize(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;
    i686_outb(0x43, 0x36);             // Command port: Square wave mode
    i686_outb(0x40, (uint8_t)(divisor & 0xFF)); // Low byte
    i686_outb(0x40, (uint8_t)((divisor >> 8) & 0xFF)); // High byte
}

void HAL_Initialize()
{
    i686_GDT_Initialize();
    i686_IDT_Initialize();
    i686_ISR_Initialize();
    i686_IRQ_Initialize();

    i686_Paging_Initialize();

    // Set PIT to 100Hz to match TIMER_FREQUENCY_HZ in time.c
    i686_PIT_Initialize(100);
}

// Hardware Abstraction Layer