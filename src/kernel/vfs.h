#pragma once
#include <stdint.h>
#include <stdbool.h>

// Forward declaration
struct VFS_File;

// The interface that every file system driver must implement
typedef struct {
    const char* Name;
    struct VFS_File* (*Open)(const char* path, const char* mode);
    uint32_t (*Read)(struct VFS_File* file, uint32_t size, void* buffer);
    uint32_t (*Write)(struct VFS_File* file, uint32_t size, const void* buffer);
    bool (*Seek)(struct VFS_File* file, uint32_t offset);
    void (*Close)(struct VFS_File* file);
} VFS_Driver;

// The generic file handle used by applications
typedef struct VFS_File {
    VFS_Driver* Driver;
    void* InternalData; // Driver specific data (e.g. FAT_File*)
} VFS_File;

void VFS_Initialize();
void VFS_Mount(const char* path, VFS_Driver* driver);

VFS_File* VFS_Open(const char* path, const char* mode);
uint32_t VFS_Read(VFS_File* file, uint32_t size, void* buffer);
uint32_t VFS_Write(VFS_File* file, uint32_t size, const void* buffer);
bool VFS_Seek(VFS_File* file, uint32_t offset);
void VFS_Close(VFS_File* file);