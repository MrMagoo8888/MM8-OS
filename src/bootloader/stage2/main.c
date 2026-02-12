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
        draw_pixel(500, 200, 0x00FF0000); // Draw a RED pixel
    }

    draw_pixel(200, 200, 0x00FFFF00); // Draw a YELLOW pixel

    if (!FAT_Initialize(&disk))
    {
        printf("FAT init error\r\n");
        goto end;
        draw_pixel(500, 300, 0x000000FF); // Draw a Blue pixel
    }

    draw_pixel(250, 250, 0x0000FFFF); // Draw a CYAN pixel

    // load kernel
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    if (!fd)
    {
        draw_pixel(500, 400, 0x00FFFF00); // Draw a YELLOW pixel
        printf("FAT open error\r\n");
        goto end;
    }
    draw_pixel(300, 300, 0x00FF00FF); // Draw a MAGENTA pixel
    uint32_t read;
    draw_pixel(350, 350, 0x00AABBFF); // Draw a light-pink pixel
    uint8_t* kernelBuffer = Kernel;
    draw_pixel(400, 400, 0x00FFAAFF); // Draw a light-purple pixel
    while ((read = FAT_Read(&disk, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer)))
    {
        memcpy(kernelBuffer, KernelLoadBuffer, read);
        kernelBuffer += read;
        draw_pixel(450, 450, 0x00FF00FF); // Draw a light-magenta pixel
    }
    draw_pixel(500, 500, 0x00FFAA00); // Draw a orange pixel
    FAT_Close(fd);
    draw_pixel(550, 550, 0x0000AAFF); // Draw a light-blue pixel

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

    // DEBUG: Dump the first 16 bytes of the loaded kernel to text (check emulator logs/memory)
    printf("Kernel @ 0x%x: ", (uint32_t)kernelStart);
    uint8_t* kPtr = (uint8_t*)kernelStart;
    for(int i = 0; i < 16; i++) printf("%x ", kPtr[i]);
    printf("\r\n");

    // DEBUG: Visual Checks
    // If WHITE pixel appears below the pink one: You have an ELF file, not a flat binary.
    if (Kernel[0] == 0x7F && Kernel[1] == 'E' && Kernel[2] == 'L' && Kernel[3] == 'F') {
        draw_pixel(600, 620, 0xFFFFFFFF); 
        // Read entry point from ELF header (offset 24 for 32-bit ELF)
        kernelStart = (KernelStart)(*(uint32_t*)(Kernel + 24));
    }
    
    // If GREY pixel appears: The kernel memory is empty (load failed or wrong address).
    if (Kernel[0] == 0x00 && Kernel[1] == 0x00) {
        draw_pixel(620, 620, 0xFF888888); 
    }

    draw_pixel(600, 600, 0x00FF00FF); // Draw a light-magenta pixel
      __asm__ volatile (
        "push %%ebx\n\t"
        "push %%ecx\n\t"
        "call *%%eax\n\t"
        "add $8, %%esp"
        : /* no outputs */
        : "b"((uint32_t)bootDrive), "c"(&vbe_screen), "a"(kernelStart)
        : "memory", "cc"
    );
    

    draw_pixel(650, 650, 0x00FF0000); // Draw a bright red pixel


    // crash for testing
    // __asm__ volatile ("int $0x13");
    
end:
    for (;;);
}



// testing