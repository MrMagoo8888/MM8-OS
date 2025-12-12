#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

#define MEMORY_MIN          0x00000500
#define MEMORY_MAX          0x00080000

// stage2 bootloader is loaded by stage1 at 0x7E00. We have space up to 0x20000.
// Let's place our data structures after the bootloader code/stack.

// FAT driver data structures
#define MEMORY_FAT_ADDR     ((void*)0x10000)
#define MEMORY_FAT_SIZE     0x10000

// Temporary buffer for loading kernel from disk
#define MEMORY_LOAD_KERNEL  ((void*)0x20000)
#define MEMORY_LOAD_SIZE    0x00010000

// 0x00080000 - 0x0009FFFF - Extended BIOS data area
// 0x000A0000 - 0x000C7FFF - Video
// 0x000C8000 - 0x000FFFFF - BIOS

#define MEMORY_KERNEL_ADDR  ((void*)0x100000)