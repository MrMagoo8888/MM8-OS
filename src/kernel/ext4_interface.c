/*
 * ext4_interface.c
 * Interface between MM8-OS diskio and lwext4 library
 */

#include "stdint.h"
#include "stdbool.h"
#include "fs/ext4.h"       /* lwext4 main header */
#include "fs/ext4_errno.h"
#include "disk.h"     /* Your existing low-level disk driver */
#include "globals.h"
#include "vfs.h"
#include "heap.h"

/* 
 * Configuration for the block device.
 * Assuming Physical Drive 0x80 (Primary Master) for this example.
 */
#define EXT2_DRIVE_NUM  0x80
#define SECTOR_SIZE    512

/*
 * Block device open: Initialize the low-level driver
 */
static int mm8_bd_open(struct ext4_blockdev *bdev)
{
    if (!DISK_Initialize(&g_Disk, EXT2_DRIVE_NUM)) {
        return EIO;
    }
    return EOK;
}

/*
 * Block device read
 */
static int mm8_bd_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id, uint32_t blk_cnt)
{
    uint8_t* data_ptr = (uint8_t*)buf;
    uint32_t current_lba = (uint32_t)blk_id;

    while (blk_cnt > 0) {
        // The ATA sector count is 8-bit. A count of 0 means 256 sectors.
        // We limit to 128 to avoid issues with 0 representing 256 in some driver implementations
        uint32_t sectors_this_call = (blk_cnt >= 128) ? 128 : blk_cnt;
        uint8_t ata_count = (uint8_t)sectors_this_call;

        if (!DISK_ReadSectors(&g_Disk, current_lba, ata_count, data_ptr)) {
            return EIO;
        }

        current_lba += sectors_this_call;
        data_ptr += sectors_this_call * SECTOR_SIZE;
        blk_cnt -= sectors_this_call;
    }
    return EOK;
}

/*
 * Block device write
 */
static int mm8_bd_bwrite(struct ext4_blockdev *bdev, const void *buf, uint64_t blk_id, uint32_t blk_cnt)
{
    const uint8_t* data_ptr = (const uint8_t*)buf;
    uint32_t current_lba = (uint32_t)blk_id;

    while (blk_cnt > 0) {
        // The ATA sector count is 8-bit. A count of 0 means 256 sectors.
        // We limit to 128 to avoid issues with 0 representing 256 in some driver implementations
        uint32_t sectors_this_call = (blk_cnt >= 128) ? 128 : blk_cnt;
        uint8_t ata_count = (uint8_t)sectors_this_call;

        if (!DISK_WriteSectors(&g_Disk, current_lba, ata_count, data_ptr)) {
            return EIO;
        }

        current_lba += sectors_this_call;
        data_ptr += sectors_this_call * SECTOR_SIZE;
        blk_cnt -= sectors_this_call;
    }
    return EOK;
}

/*
 * Block device close
 */
static int mm8_bd_close(struct ext4_blockdev *bdev)
{
    /* Usually nothing to do for simple disk drivers */
    return EOK;
}

/*
 * Define the interface structure required by lwext4
 */
struct ext4_blockdev_iface mm8_bd_iface = {
    .open = mm8_bd_open,
    .bread = mm8_bd_bread,
    .bwrite = mm8_bd_bwrite,
    .close = mm8_bd_close,
    .lock = NULL,   /* Implement if you have multitasking/mutexes */
    .unlock = NULL, /* Implement if you have multitasking/mutexes */
    .ph_bsize = SECTOR_SIZE,
    .ph_bcnt = 0,   /* Filled during initialization usually, or set manually */
};

/*
 * The block device structure instance
 */
struct ext4_blockdev mm8_ext4_fs;

/*
 * Public function to initialize and mount ext2
 */
int init_ext2_filesystem(void)
{
    int r;

    /* 1. Setup block device structure */
    mm8_ext4_fs.bdif = &mm8_bd_iface;
    mm8_ext4_fs.part_offset = 0; /* Set this to the LBA offset of your ext2 partition! */
    mm8_ext4_fs.part_size = 0;   /* Set to partition size or 0 for auto-detect if supported */

    /* 2. Register the device with lwext4 */
    r = ext4_device_register(&mm8_ext4_fs, "ext2_disk");
    if (r != EOK) {
        return r;
    }

    /* 3. Mount the filesystem to a mount point (e.g., "/") */
    /* Note: lwext4 requires you to call ext4_mount with the device name */
    r = ext4_mount("ext2_disk", "/", false);
    
    if (r != EOK) {
        return r;
    }

    return 0; // Success
}

/*
 * VFS Adapter for lwext4
 */

static VFS_File* Ext4_Open(const char* path, const char* mode) {
    ext4_file* fp = (ext4_file*)malloc(sizeof(ext4_file));
    if (!fp) return NULL;

    if (ext4_fopen(fp, path, mode) != EOK) {
        free(fp);
        return NULL;
    }

    VFS_File* file = (VFS_File*)malloc(sizeof(VFS_File));
    if (!file) {
        ext4_fclose(fp);
        free(fp);
        return NULL;
    }
    
    file->InternalData = fp;
    ext4_fseek(fp, 0, SEEK_END);
    file->Size = (uint32_t)ext4_ftell(fp);
    ext4_fseek(fp, 0, SEEK_SET);
    return file;
}

static void Ext4_Close(VFS_File* file) {
    if (file && file->InternalData) {
        ext4_file* fp = (ext4_file*)file->InternalData;
        ext4_fclose(fp);
        free(fp);
    }
}

static uint32_t Ext4_Read(VFS_File* file, uint32_t size, void* buffer) {
    if (!file || !file->InternalData) return 0;
    ext4_file* fp = (ext4_file*)file->InternalData;
    size_t br;
    if (ext4_fread(fp, buffer, size, &br) != EOK) return 0;
    return (uint32_t)br;
}

static uint32_t Ext4_Write(VFS_File* file, uint32_t size, const void* buffer) {
    if (!file || !file->InternalData) return 0;
    ext4_file* fp = (ext4_file*)file->InternalData;
    size_t bw;
    if (ext4_fwrite(fp, buffer, size, &bw) != EOK) return 0;
    return (uint32_t)bw;
}

static bool Ext4_Seek(VFS_File* file, uint32_t offset) {
    if (!file || !file->InternalData) return false;
    ext4_file* fp = (ext4_file*)file->InternalData;
    return ext4_fseek(fp, offset, SEEK_SET) == EOK;
}

VFS_Driver g_Ext4Driver = {
    .Name = "EXT4",
    .Open = Ext4_Open,
    .Read = Ext4_Read,
    .Write = Ext4_Write,
    .Seek = Ext4_Seek,
    .Close = Ext4_Close
};