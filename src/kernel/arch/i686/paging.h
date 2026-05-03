#pragma once
#include <stdint.h>

// Page Directory Entry / Page Table Entry flags
#define PAGE_PRESENT    0x01
#define PAGE_READWRITE  0x02
#define PAGE_USER       0x04

void i686_Paging_Initialize();

void i686_Paging_Enable(uint32_t page_directory_phys);