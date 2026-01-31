#pragma once

#include "stdint.h"
#include "stdbool.h"

typedef struct VFS_Driver VFS_Driver;

typedef struct VFS_File {
    void* InternalData;
    VFS_Driver* Driver;
    uint32_t Size;
} VFS_File;

struct VFS_Driver {
    const char* Name;
    VFS_File* (*Open)(const char* path, const char* mode);
    void (*Close)(VFS_File* file);
    uint32_t (*Read)(VFS_File* file, uint32_t size, void* buffer);
    uint32_t (*Write)(VFS_File* file, uint32_t size, const void* buffer);
    bool (*Seek)(VFS_File* file, uint32_t offset);
};

void VFS_Initialize();
bool VFS_Mount(const char* path, VFS_Driver* driver);

VFS_File* VFS_Open(const char* path, const char* mode);
void VFS_Close(VFS_File* file);
uint32_t VFS_Read(VFS_File* file, uint32_t size, void* buffer);
uint32_t VFS_Write(VFS_File* file, uint32_t size, const void* buffer);
bool VFS_Seek(VFS_File* file, uint32_t offset);