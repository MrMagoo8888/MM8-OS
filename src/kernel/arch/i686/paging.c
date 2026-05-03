#include "paging.h"
#include "isr.h"
#include "stdio.h"
#include <arch/i686/io.h>
#include "vbe.h"

// Statically allocate the initial page directory and one page table
// Both must be 4KB aligned.
uint32_t page_directory[1024] __attribute__((aligned(4096)));

// Allocate enough page tables to cover 512MB of RAM + VBE
uint32_t page_tables[130][1024] __attribute__((aligned(4096)));

void page_fault_handler(Registers* regs) {
    // The faulting address is stored in CR2
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    printf("PAGE FAULT! Addr: 0x%x, Error: 0x%x\n", faulting_address, regs->error);
    printf("EIP: 0x%x\n", regs->eip);
    
    i686_Panic();
}

void i686_Paging_Map_Range(uint32_t virt, uint32_t phys, uint32_t size) {
    for (uint32_t i = 0; i < size; i += 4096) {
        uint32_t v_addr = virt + i;
        uint32_t p_addr = phys + i;

        uint32_t pd_idx = v_addr >> 22;
        uint32_t pt_idx = (v_addr >> 12) & 0x3FF;

        if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
            static int pt_pool_idx = 0;
            uint32_t pt_phys = (uint32_t)page_tables[pt_pool_idx++];
            page_directory[pd_idx] = pt_phys | PAGE_PRESENT | PAGE_READWRITE | 0x04; // Add USER bit
        }

        uint32_t* table = (uint32_t*)(page_directory[pd_idx] & 0xFFFFF000);
        table[pt_idx] = p_addr | PAGE_PRESENT | PAGE_READWRITE | 0x04; // Add USER bit
    }
}

void i686_Paging_Initialize() {
    // Clear directory
    for (int i = 0; i < 1024; i++) page_directory[i] = 0;

    // Register page fault handler
    i686_ISR_RegisterHandler(14, page_fault_handler);

    // 1. Identity map the first 512MB
    // This covers the kernel, stack, and the 256MB heap
    printf("Paging: Mapping 512MB Identity Map...\n");
    i686_Paging_Map_Range(0, 0, 1024 * 1024 * 512);

    // 2. Identity map the VBE Framebuffer
    // Without this, the first printf() or clrscr() after enabling paging will crash.
    if (g_vbe_screen && g_vbe_screen->physical_buffer) {
        uint32_t vbe_phys = g_vbe_screen->physical_buffer;
        uint32_t vbe_size = g_vbe_screen->height * g_vbe_screen->pitch;
        printf("Paging: Mapping VBE Framebuffer at 0x%x (Size: %u KB)...\n", vbe_phys, vbe_size / 1024);
        i686_Paging_Map_Range(vbe_phys, vbe_phys, vbe_size);
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