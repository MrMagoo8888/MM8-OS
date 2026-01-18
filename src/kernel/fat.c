#include "fat.h"
#include "ff.h"
#include "vfs.h"
#include "heap.h"
#include "string.h"
#include "stdio.h"

// --- VFS Adapter for FatFs ---

static VFS_File* FatFs_Open(const char* path, const char* mode) {
    FIL* fp = (FIL*)malloc(sizeof(FIL));
    if (!fp) return NULL;

    BYTE mode_flags = 0;
    // Simple mode mapping
    if (strchr(mode, 'r')) mode_flags |= FA_READ;
    if (strchr(mode, 'w')) mode_flags |= FA_WRITE | FA_CREATE_ALWAYS;
    if (strchr(mode, 'a')) mode_flags |= FA_WRITE | FA_OPEN_APPEND;
    // Add 'plus' modes or others if needed

    if (f_open(fp, path, mode_flags) != FR_OK) {
        free(fp);
        return NULL;
    }

    VFS_File* file = (VFS_File*)malloc(sizeof(VFS_File));
    if (!file) {
        f_close(fp);
        free(fp);
        return NULL;
    }
    
    file->InternalData = fp;
    return file;
}

static void FatFs_Close(VFS_File* file) {
    if (file && file->InternalData) {
        FIL* fp = (FIL*)file->InternalData;
        f_close(fp);
        free(fp);
        // The VFS_File struct itself is freed by the caller (VFS_Close)
    }
}

static uint32_t FatFs_Read(VFS_File* file, uint32_t size, void* buffer) {
    if (!file || !file->InternalData) return 0;
    FIL* fp = (FIL*)file->InternalData;
    UINT br;
    f_read(fp, buffer, size, &br);
    return br;
}

static uint32_t FatFs_Write(VFS_File* file, uint32_t size, const void* buffer) {
    if (!file || !file->InternalData) return 0;
    FIL* fp = (FIL*)file->InternalData;
    UINT bw;
    f_write(fp, buffer, size, &bw);
    return bw;
}

static bool FatFs_Seek(VFS_File* file, uint32_t offset) {
    if (!file || !file->InternalData) return false;
    FIL* fp = (FIL*)file->InternalData;
    return f_lseek(fp, offset) == FR_OK;
}

VFS_Driver g_FatFsDriver = {
    .Name = "FATFS",
    .Open = FatFs_Open,
    .Read = FatFs_Read,
    .Write = FatFs_Write,
    .Seek = FatFs_Seek,
    .Close = FatFs_Close
};

// --- Compatibility Layer for Legacy FAT API ---

FAT_File* FAT_Open(DISK* disk, const char* filename, int mode) {
    (void)disk;
    
    BYTE fatfs_mode = FA_READ;
    if (mode == FAT_OPEN_MODE_WRITE) {
        fatfs_mode = FA_WRITE | FA_OPEN_ALWAYS;
    } else if (mode == FAT_OPEN_MODE_CREATE) {
        fatfs_mode = FA_WRITE | FA_CREATE_ALWAYS;
    }

    FAT_File* file = (FAT_File*)malloc(sizeof(FAT_File));
    if (!file) return NULL;

    if (f_open(&file->handle, filename, fatfs_mode) != FR_OK) {
        free(file);
        return NULL;
    }
    file->Size = f_size(&file->handle);
    return file;
}

uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut) {
    (void)disk;
    UINT br;
    f_read(&file->handle, dataOut, byteCount, &br);
    return br;
}

uint32_t FAT_Write(DISK* disk, FAT_File* file, uint32_t byteCount, const void* dataIn) {
    (void)disk;
    UINT bw;
    f_write(&file->handle, dataIn, byteCount, &bw);
    return bw;
}

bool FAT_Seek(DISK* disk, FAT_File* file, uint32_t offset) {
    (void)disk;
    if (!file) return false;
    return f_lseek(&file->handle, offset) == FR_OK;
}

void FAT_Close(DISK* disk, FAT_File* file) {
    (void)disk;
    if (file) {
        f_close(&file->handle);
        free(file);
    }
}
