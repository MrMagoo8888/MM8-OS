#include "stdint.h"
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdint.h"

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

typedef void (*KernelStart)();

void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    clrscr();

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

    printf("FAT initialized\r\n");

    // load kernel
    printf("Loading kernel...\r\n");
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    if (fd == NULL) {
        printf("Failed to open kernel.bin\r\n");
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

    // execute kernel
    printf("Jumping to kernel...\r\n");
    KernelStart kernelStart = (KernelStart)Kernel;
    kernelStart();

end:
    for (;;);
}

// testing