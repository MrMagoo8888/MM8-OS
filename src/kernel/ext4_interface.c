/*
 * ext4_interface.c
 * Interface between MM8-OS diskio and lwext4 library
 */

#include <stdint.h>
#include <stdbool.h>
#include "fs/ext4.h"       /* lwext4 main header */
#include "fs/ext4_errno.h"
#include "diskio.h"     /* Your existing low-level disk driver */

/* 
 * Configuration for the block device.
 * Assuming Physical Drive 0 for this example.
 */
#define EXT2_PDRV_NUM  0
#define SECTOR_SIZE    512

/*
 * Block device open: Initialize the low-level driver
 */
static int mm8_bd_open(struct ext4_blockdev *bdev)
{
    DSTATUS status = disk_initialize(EXT2_PDRV_NUM);
    if (status & STA_NOINIT) {
        return EIO;
    }
    return EOK;
}

/*
 * Block device read
 */
static int mm8_bd_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id, uint32_t blk_cnt)
{
    DRESULT res;
    
    /* disk_read takes: pdrv, buffer, sector, count */
    res = disk_read(EXT2_PDRV_NUM, (BYTE*)buf, (LBA_t)blk_id, (UINT)blk_cnt);
    
    if (res != RES_OK) {
        return EIO;
    }
    return EOK;
}

/*
 * Block device write
 */
static int mm8_bd_bwrite(struct ext4_blockdev *bdev, const void *buf, uint64_t blk_id, uint32_t blk_cnt)
{
    DRESULT res;
    
    /* disk_write takes: pdrv, buffer, sector, count */
    res = disk_write(EXT2_PDRV_NUM, (const BYTE*)buf, (LBA_t)blk_id, (UINT)blk_cnt);
    
    if (res != RES_OK) {
        return EIO;
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