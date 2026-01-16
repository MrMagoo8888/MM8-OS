#include "vfs.h"
#include "string.h"
#include "stdio.h"
#include "heap.h"

// Simple mount point system: Root is FAT for now.
static VFS_Driver* g_RootDriver = NULL;

void VFS_Initialize() {
    g_RootDriver = NULL;
}

void VFS_Mount(const char* path, VFS_Driver* driver) {
    // For simplicity in this stage, we only support mounting to root "/"
    // A full VFS would have a tree of mount points.
    if (strcmp(path, "/") == 0) {
        g_RootDriver = driver;
    }
}

VFS_File* VFS_Open(const char* path, const char* mode) {
    if (!g_RootDriver) return NULL;
    
    // Delegate to root driver
    VFS_File* file = g_RootDriver->Open(path, mode);
    if (file) {
        file->Driver = g_RootDriver;
    }
    return file;
}

uint32_t VFS_Read(VFS_File* file, uint32_t size, void* buffer) {
    if (!file || !file->Driver || !file->Driver->Read) return 0;
    return file->Driver->Read(file, size, buffer);
}

uint32_t VFS_Write(VFS_File* file, uint32_t size, const void* buffer) {
    if (!file || !file->Driver || !file->Driver->Write) return 0;
    return file->Driver->Write(file, size, buffer);
}

bool VFS_Seek(VFS_File* file, uint32_t offset) {
    if (!file || !file->Driver || !file->Driver->Seek) return false;
    return file->Driver->Seek(file, offset);
}

void VFS_Close(VFS_File* file) {
    if (!file) return;
    
    if (file->Driver && file->Driver->Close) {
        file->Driver->Close(file);
    }
    
    // Free the VFS wrapper handle
    // Note: The driver is responsible for freeing InternalData if it allocated it dynamically
    free(file);
}