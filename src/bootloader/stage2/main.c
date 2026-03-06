#include "stdint.h"
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "vbe.h"
#include "graphics.h"
#include "stddef.h"

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

typedef void (*KernelStart)(VbeScreenInfo* vbe_info, uint16_t bootDrive);

void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    // clrscr() is for text mode, so we remove it.
    draw_pixel(100, 100, 0x0000FF00); // Draw a GREEN pixel
    draw_pixel(150, 150, 0x000000FF); // Draw a BLUE pixel


    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        draw_pixel(100, 800, 0x00FF0000); // Draw a RED pixel
        goto end;
    }
    draw_pixel(200, 200, 0x00FF6699); // Draw a PINK pixel

    if (!FAT_Initialize(&disk))
    {
        printf("FAT init error\r\n");
        draw_pixel(200, 800, 0x00123456); // Draw a deep sea blue pixel
        goto end;
    }
    draw_pixel(250, 250, 0x00FFFF00); // Draw a YELLOW pixel

    // load kernel
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    if (fd == NULL)
    {
        printf("Kernel not found\r\n");
        draw_pixel(250, 800, 0x00FF0000); // Draw a RED pixel
        goto end;
    }

    uint32_t read;
    uint8_t* kernelBuffer = Kernel;
    while ((read = FAT_Read(&disk, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer)))
    {
        memcpy(kernelBuffer, KernelLoadBuffer, read);
        kernelBuffer += read;
    }
    FAT_Close(fd);
    draw_pixel(300, 300, 0x00FF00FF); // Draw a MAGENTA pixel

    // Prepare to execute the kernel
    // We will pass a pointer to the vbe_screen info structure in the EAX register
    // and the boot drive in the EBX register.

    printf("Bootloader VBE Info:\r\n");
    printf("  Width: %u\r\n", vbe_screen.width);
    printf("  Height: %u\r\n", vbe_screen.height);
    printf("  Pitch: %u\r\n", vbe_screen.pitch);
    printf("  BPP: %u\r\n", vbe_screen.bpp);
    printf("  Physical Buffer: 0x%x\r\n", vbe_screen.physical_buffer);
    draw_pixel(350, 350, 0x0000FFFF); // Draw a CYAN pixel
    KernelStart kernelStart = (KernelStart)Kernel; // Kernel's entry point
    kernelStart(&vbe_screen, bootDrive);

    draw_pixel(300, 800, 0x0000FFFF); // Draw a CYAN pixel
end:
    for (;;);
}

// testing