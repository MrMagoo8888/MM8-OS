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

void HAL_Speaker_Play(uint32_t frequency) {
    if (frequency == 0) return;
    
    uint32_t divisor = 1193180 / frequency;
    i686_outb(0x43, 0xB6); // Command: Channel 2, LSB/MSB, Square Wave
    i686_outb(0x42, (uint8_t)(divisor & 0xFF));
    i686_outb(0x42, (uint8_t)((divisor >> 8) & 0xFF));

    uint8_t status = i686_inb(0x61);
    if (status != (status | 3)) {
        i686_outb(0x61, status | 3); // Set bits 0 and 1
    }
}

void HAL_Speaker_Stop() {
    uint8_t status = i686_inb(0x61) & 0xFC; // Clear bits 0 and 1
    i686_outb(0x61, status);
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

    __asm__ volatile("sti"); // Enable interrupts only after all hardware tables are ready
}

// Hardware Abstraction Layer