#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "disk.h"
#include "vfs.h"
#include "ff.h"

// The VFS driver instance for FatFs
extern VFS_Driver g_FatFsDriver;

// Compatibility layer for legacy FAT driver API
typedef struct {
    FIL handle;
    uint32_t Size;
} FAT_File;
#define FAT_OPEN_MODE_READ 0
#define FAT_OPEN_MODE_WRITE 1
#define FAT_OPEN_MODE_CREATE 2

FAT_File* FAT_Open(DISK* disk, const char* filename, int mode);
uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut);
uint32_t FAT_Write(DISK* disk, FAT_File* file, uint32_t byteCount, const void* dataIn);
bool FAT_Seek(DISK* disk, FAT_File* file, uint32_t offset);
void FAT_Close(DISK* disk, FAT_File* file);