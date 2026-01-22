#include "vfs.h"
#include "string.h"
#include "stdio.h"
#include "heap.h"

// Simple VFS implementation supporting only root mount for now
static VFS_Driver* g_RootDriver = NULL;

void VFS_Initialize() {
    g_RootDriver = NULL;
}

bool VFS_Mount(const char* path, VFS_Driver* driver) {
    // For simplicity, we only support mounting to root "/"
    if (strcmp(path, "/") == 0) {
        g_RootDriver = driver;
        return true;
    }
    printf("VFS: Only root mount is supported currently.\n");
    return false;
}

VFS_File* VFS_Open(const char* path, const char* mode) {
    if (!g_RootDriver) return NULL;
    
    // Delegate to the driver
    VFS_File* file = g_RootDriver->Open(path, mode);
    if (file) {
        file->Driver = g_RootDriver;
    }
    return file;
}

void VFS_Close(VFS_File* file) {
    if (file) {
        if (file->Driver && file->Driver->Close) {
            file->Driver->Close(file);
        }
        // Free the VFS_File wrapper itself (allocated by the driver's Open)
        free(file);
    }
}

uint32_t VFS_Read(VFS_File* file, uint32_t size, void* buffer) {
    if (file && file->Driver && file->Driver->Read) {
        return file->Driver->Read(file, size, buffer);
    }
    return 0;
}

uint32_t VFS_Write(VFS_File* file, uint32_t size, const void* buffer) {
    if (file && file->Driver && file->Driver->Write) {
        return file->Driver->Write(file, size, buffer);
    }
    return 0;
}

bool VFS_Seek(VFS_File* file, uint32_t offset) {
    if (file && file->Driver && file->Driver->Seek) {
        return file->Driver->Seek(file, offset);
    }
    return false;
}