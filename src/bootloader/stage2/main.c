#include "stdint.h"
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdint.h"
#include "vbe.h"
#include "../../common/bootinfo.h"

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

// Define the kernel's entry point function signature
// It takes one argument: a pointer to the VBE_INFO structure.
typedef void (*KernelStart)(VBE_INFO*);


void __attribute__((cdecl)) c_start_bootloader(uint16_t bootDrive)
{
    // Draw a pixel to show we've entered C code
    draw_pixel(310, 50, 0x00FF00FF); // Magenta

    /*draw_pixel(500, 600, 0x00FFFFFF); // White corner
    draw_pixel(501, 600, 0x00FFFFFF); // White along one
    draw_pixel(502, 600, 0x00FFFFFF); // White along one
    draw_pixel(503, 600, 0x00FFFFFF); // White along other */

    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    // Draw a pixel to show disk was initialized
    draw_pixel(320, 50, 0x00FFFF00); // Yellow

    if (!FAT_Initialize(&disk))
    {
        printf("FAT init error\r\n");
        draw_pixel(300, 100, 0x00F0F12F5); // Light Blue
        goto end;
    }

    // Draw a pixel to show FAT was initialized
    draw_pixel(330, 50, 0x0000FFFF); // Cyan

    printf("FAT initialized\r\n");

    // load kernel
    printf("Loading kernel...\r\n");
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    if (fd == NULL) {
        printf("Failed to open kernel.bin\r\n");
        draw_pixel(310, 100, 0x00FFFFFF); // White
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

    printf("Kernel loaded successfully.\r\n");

    // Draw a pixel to show kernel was loaded
    draw_pixel(340, 50, 0x0000FF00); // Green

    // execute kernel
    printf("Jumping to kernel...\r\n");
    // Draw a pixel to show we are about to jump
    draw_pixel(350, 50, 0x00FF0000); // Red

    // The VBE info is at a fixed address (see bootinfo.h)
    VBE_INFO* vbe_info = (VBE_INFO*)BOOTINFO_ADDR;

    KernelStart kernelStart = (KernelStart)((uint32_t)Kernel);
    kernelStart(vbe_info);

end:
    for (;;);
}

// testing