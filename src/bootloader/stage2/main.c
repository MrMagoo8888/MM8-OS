#include "stdint.h"
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "vbe.h"
#include "graphics.h"

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

typedef void (*KernelStart)();

void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    // clrscr() is for text mode, so we remove it.
    draw_pixel(100, 100, 0x0000FF00); // Draw a GREEN pixel
    draw_pixel(150, 150, 0x000000FF); // Draw a BLUE pixel


    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    if (!FAT_Initialize(&disk))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    // load kernel
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    uint32_t read;
    uint8_t* kernelBuffer = Kernel;
    while ((read = FAT_Read(&disk, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer)))
    {
        memcpy(kernelBuffer, KernelLoadBuffer, read);
        kernelBuffer += read;
    }
    FAT_Close(fd);

    // Prepare to execute the kernel
    // We will pass a pointer to the vbe_screen info structure in the EAX register
    // and the boot drive in the EBX register.

    printf("Bootloader VBE Info:\r\n");
    printf("  Width: %u\r\n", vbe_screen.width);
    printf("  Height: %u\r\n", vbe_screen.height);
    printf("  Pitch: %u\r\n", vbe_screen.pitch);
    printf("  BPP: %u\r\n", vbe_screen.bpp);
    printf("  Physical Buffer: 0x%x\r\n", vbe_screen.physical_buffer);
    KernelStart kernelStart = (KernelStart)Kernel; // Kernel's entry point
    __asm__ volatile (
        "push %0\n\t"
        "push %1\n\t"
        "call *%2\n\t"
        "add $8, %%esp"
        : /* no outputs */
        : "r"((uint32_t)bootDrive), "r"(&vbe_screen), "r"(kernelStart)
        : "memory"
    );

end:
    for (;;);
}

// testing