#include "disk.h"
#include "arch/i686/io.h"
#include "stdio.h"

// ATA PIO port definitions
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERROR        0x1F1
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_CONTROL      0x3F6 // Device Control Register
#define ATA_PRIMARY_COMMAND      0x1F7

// ATA status register flags
#define ATA_STATUS_BUSY          0x80
#define ATA_STATUS_DRIVE_READY   0x40
#define ATA_STATUS_DATA_REQUEST  0x08
#define ATA_STATUS_ERROR         0x01

// ATA commands
#define ATA_CMD_READ_SECTORS     0x20


// This is a placeholder for a real ATA PIO driver.
// For now, it does nothing but provides the interface for the FAT driver.

bool DISK_Initialize(DISK* disk, uint8_t driveNumber) {
    disk->id = driveNumber;
    // This is a placeholder. It doesn't interact with hardware.
    printf("DISK: Initializing disk %d (placeholder)\n", driveNumber);
    return true;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t count, void* buffer) {
    // This is a placeholder. It will always fail to read.
    // This will cause the FAT initialization to fail, which is expected.
    printf("DISK: ReadSectors(lba=%u, count=%u) - NOT IMPLEMENTED\n", lba, count);
    return false;
}