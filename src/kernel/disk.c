#include "disk.h"
#include "arch/i686/io.h"
#include "stdio.h"
#include "stddef.h"
#include "time.h"
#include "hal/usb_msc.h"
#include "hal/ehci.h"

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
#define ATA_CMD_WRITE_SECTORS    0x30
#define ATA_CMD_CACHE_FLUSH      0xE7
#define ATA_CMD_IDENTIFY_DEVICE  0xEC

// Internal ATA helper
static bool ATA_Initialize(DISK* disk, uint8_t driveNumber) {
    if (driveNumber < 0x80) return false;

    // --- Stage 1: Software Reset ---
    // Select the master drive
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    i686_iowait();

    // Perform a software reset
    i686_outb(ATA_PRIMARY_CONTROL, 0x04); // Set SRST (Software Reset)
    i686_iowait();
    i686_outb(ATA_PRIMARY_CONTROL, 0x00); // Clear SRST
    i686_iowait();

    // Wait for the drive to finish the reset.
    int timeout = 100000; 
    while ((i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BUSY) && --timeout);
    if (timeout == 0) {
        printf("DISK: Controller reset timeout.\n");
        return false;
    }

    // --- Stage 2: IDENTIFY command ---
    // Select the master drive again
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);

    // Zero out registers
    i686_outb(ATA_PRIMARY_SECTOR_COUNT, 0);
    i686_outb(ATA_PRIMARY_LBA_LOW, 0);
    i686_outb(ATA_PRIMARY_LBA_MID, 0);
    i686_outb(ATA_PRIMARY_LBA_HIGH, 0);

    // Send IDENTIFY DEVICE command
    i686_outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY_DEVICE);
    i686_iowait();

    // Check for drive presence
    if (i686_inb(ATA_PRIMARY_STATUS) == 0) {
        printf("DISK: No drive found on primary master.\n");
        return false;
    }

    // Poll for BSY to clear and DRQ or ERR to be set
    timeout = 100000;
    while ((i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BUSY) && --timeout);

    timeout = 100000;
    while (!(i686_inb(ATA_PRIMARY_STATUS) & (ATA_STATUS_DATA_REQUEST | ATA_STATUS_ERROR)) && --timeout);

    if (timeout == 0) return false;

    if (i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERROR) {
        printf("DISK: Drive %d (ATA) not ready.\n", driveNumber - 0x80);
        return false;
    }

    // Read and discard the 512-byte identification data
    uint16_t identify_data[256];
    i686_insw(ATA_PRIMARY_DATA, identify_data, 256);

    disk->type = DISK_TYPE_ATA;
    disk->id = driveNumber - 0x80;

    printf("DISK: Initialized drive %d (ATA).\n", disk->id);
    return true;
}

bool DISK_Initialize(DISK* disk, uint8_t driveNumber) {
    // 1. Check if the USB driver (OHCI) already claimed this disk during PCI enumeration
    if (disk->type == DISK_TYPE_USB && disk->driver_data != NULL) {
        printf("DISK: Using USB Mass Storage for drive 0x%x\n", driveNumber);
        disk->id = driveNumber - 0x80;
        return true;
    }

    // 2. Otherwise, try to initialize as an ATA drive
    if (ATA_Initialize(disk, driveNumber)) {
        return true;
    }

    return false;
}

extern int usb_msc_read_sectors(ehci_controller_t* hc, uint32_t lba, uint8_t count, void* buffer);

static bool USB_ReadSectors(DISK* disk, uint32_t lba, uint8_t count, void* buffer) {
    if (!disk->driver_data) return false;
    ehci_controller_t* hc = (ehci_controller_t*)disk->driver_data;
    
    // From Scratch: USB Mass Storage often requires multiple retries 
    // if the drive is spinning up.
    for (int i = 0; i < 3; i++) {
        if (usb_msc_read_sectors(hc, lba, count, buffer) == 0) return true;
        printf("USB: Read failed, retry %d...\n", i + 1);
        sleep_ms(100);
    }
    return false;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t count, void* buffer) {
    if (disk->type == DISK_TYPE_USB) {
        return USB_ReadSectors(disk, lba, count, buffer);
    }

    // Wait until the drive is not busy
    int timeout = 0x0FFFFFFF;
    while ((i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BUSY) && --timeout);

    // Select drive (Master) and send LBA bits 24-27
    // 0xE0 for master drive in LBA mode
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | (disk->id << 4) | ((lba >> 24) & 0x0F));
    // Send the number of sectors to read
    i686_outb(ATA_PRIMARY_SECTOR_COUNT, count);

    // Send the LBA address (bits 0-23)
    i686_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    i686_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    i686_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));

    // Send the READ SECTORS command
    i686_outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);

    uint16_t* target = (uint16_t*)buffer;

    for (int i = 0; i < count; i++) {
        // Poll until the drive is ready to transfer data (BSY clear, DRQ set)
        timeout = 0x0FFFFFFF;
        while (!((i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_DATA_REQUEST) || (i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERROR)) && --timeout);

        // Check for an error
        if (i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERROR) {
            printf("DISK: Read error! LBA=%u, Count=%u\n", lba, count);
            return false; // Error occurred
        }

        // Read 256 16-bit words (512 bytes) from the data port into the buffer
        i686_insw(ATA_PRIMARY_DATA, target, 256);
        target += 256;
    }
    return true;
}

bool DISK_WriteSectors(DISK* disk, uint32_t lba, uint8_t count, const void* buffer) {
    if (disk->type == DISK_TYPE_USB) {
        return false; // Implement USB_WriteSectors later
    }

    // Wait until the drive is not busy
    int timeout = 0x0FFFFFFF;
    while ((i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BUSY) && --timeout);

    // Select drive (Master) and send LBA bits 24-27
    // 0xE0 for master drive in LBA mode
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | (disk->id << 4) | ((lba >> 24) & 0x0F));
    // Send the number of sectors to write
    i686_outb(ATA_PRIMARY_SECTOR_COUNT, count);

    // Send the LBA address (bits 0-23)
    i686_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    i686_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    i686_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));

    // Send the WRITE SECTORS command
    i686_outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);

    const uint16_t* source = (const uint16_t*)buffer;

    for (int i = 0; i < count; i++) {
        // Poll until the drive is ready to receive data (BSY clear, DRQ set)
        timeout = 0x0FFFFFFF;
        while (!((i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_DATA_REQUEST) || (i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERROR)) && --timeout);

        // Check for an error
        if (i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERROR) {
            printf("DISK: Write error! LBA=%u, Count=%u\n", lba, count);
            return false; // Error occurred
        }

        // Write 256 16-bit words (512 bytes) from the buffer to the data port
        i686_outsw(ATA_PRIMARY_DATA, source, 256);
        source += 256;
    }

    // Flush the cache to ensure data is written to the disk platter
    i686_outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);
    timeout = 0x0FFFFFFF;
    while ((i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BUSY) && --timeout);

    return true;
}