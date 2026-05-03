#include "paging.h"
#include "isr.h"
#include "stdio.h"
#include <arch/i686/io.h>

// Statically allocate the initial page directory and one page table
// Both must be 4KB aligned.
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void page_fault_handler(Registers* regs) {
    // The faulting address is stored in CR2
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    printf("PAGE FAULT! Addr: 0x%x, Error: 0x%x\n", faulting_address, regs->error);
    printf("EIP: 0x%x\n", regs->eip);
    
    i686_Panic();
}

void i686_Paging_Initialize() {
    // Register page fault handler
    i686_ISR_RegisterHandler(14, page_fault_handler);

    // 1. Identity map the first 4MB
    // This ensures the kernel code (usually at 0x100000 or lower) remains valid
    for (int i = 0; i < 1024; i++) {
        // Map address (i * 4KB) with Present and Read/Write flags
        first_page_table[i] = (i * 4096) | (PAGE_PRESENT | PAGE_READWRITE);
    }

    // 2. Setup the Page Directory
    // First entry points to our identity-mapped table
    page_directory[0] = ((uint32_t)first_page_table) | (PAGE_PRESENT | PAGE_READWRITE);

    // Set all other entries to "not present"
    for (int i = 1; i < 1024; i++) {
        page_directory[i] = PAGE_READWRITE; // Not present, but R/W
    }

    // 3. Enable Paging
    printf("Paging: Enabling identity map for first 4MB...\n");
    i686_Paging_Enable((uint32_t)page_directory);
}

void i686_Paging_Enable(uint32_t page_directory_phys) {
    __asm__ volatile (
        "mov %0, %%cr3\n\t"        // Load Page Directory Base Register (CR3)
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t" // Set PG bit (31)
        "mov %%eax, %%cr0\n\t"
        :
        : "r"(page_directory_phys)
        : "eax", "memory"
    );
}