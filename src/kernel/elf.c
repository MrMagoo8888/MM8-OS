#include "elf.h"
#include "fat.h"
#include "stdio.h"
#include "memory.h"
#include "globals.h"

void* elf_load(const char* path) {
    FAT_File* file = FAT_Open(&g_Disk, path, FAT_OPEN_MODE_READ);
    if (!file) {
        printf("ELF: Could not open %s\n", path);
        return NULL;
    }

    Elf32_Ehdr header;
    if (FAT_Read(&g_Disk, file, sizeof(Elf32_Ehdr), &header) != sizeof(Elf32_Ehdr)) {
        printf("ELF: Failed to read header\n");
        FAT_Close(&g_Disk, file);
        return NULL;
    }

    // Verify Magic
    if (*(uint32_t*)header.e_ident != ELF_MAGIC) {
        printf("ELF: Invalid magic number\n");
        FAT_Close(&g_Disk, file);
        return NULL;
    }

    // Parse Program Headers
    for (int i = 0; i < header.e_phnum; i++) {
        Elf32_Phdr phdr;
        uint32_t phdr_offset = header.e_phoff + (i * header.e_phentsize);
        
        FAT_Seek(&g_Disk, file, phdr_offset);
        FAT_Read(&g_Disk, file, sizeof(Elf32_Phdr), &phdr);

        if (phdr.p_type == PT_LOAD) {
            // WARNING: In a real OS, you'd map these pages using paging.
            // Here, we trust the ELF doesn't overwrite the kernel (0-4MB).
            if (phdr.p_vaddr < 0x1000000) {
                printf("ELF: Security violation - segment linked to kernel space\n");
                FAT_Close(&g_Disk, file);
                return NULL;
            }

            // Move to segment data in file
            FAT_Seek(&g_Disk, file, phdr.p_offset);

            // Load data from file
            printf("ELF: Loading segment at 0x%x (%u bytes)\n", phdr.p_vaddr, phdr.p_filesz);
            
            // Note: This assumes identity mapping has enough space!
            uint8_t* segment_ptr = (uint8_t*)phdr.p_vaddr;
            FAT_Read(&g_Disk, file, phdr.p_filesz, segment_ptr);

            // Handle BSS (memory size > file size)
            if (phdr.p_memsz > phdr.p_filesz) {
                memset(segment_ptr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
            }
        }
    }

    FAT_Close(&g_Disk, file);
    return (void*)header.e_entry;
}