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
#define ATA_CMD_WRITE_SECTORS    0x30
#define ATA_CMD_CACHE_FLUSH      0xE7
#define ATA_CMD_IDENTIFY_DEVICE  0xEC


// This is a placeholder for a real ATA PIO driver.
// For now, it does nothing but provides the interface for the FAT driver.

// The ATA spec requires a 400ns delay after certain operations.
// Reading the Alternate Status register 4 times is the standard way to achieve this.
static void ATA_Wait400ns(void) {
    // Reading the Alternate Status register is safer than Status
    // because it doesn't affect the interrupt state.
    for (int i = 0; i < 4; i++) {
        i686_inb(ATA_PRIMARY_CONTROL);
    }
}

bool DISK_Initialize(DISK* disk, uint8_t driveNumber) {
    if (driveNumber < 0x80) {
        printf("DISK: Cannot initialize floppy drive %d with ATA driver.\n", driveNumber);
        return false;
    }

    // Translate BIOS drive number (e.g., 0x80 for hda) to ATA drive ID (0 for master)
    disk->id = driveNumber - 0x80;
    disk->type = DISK_TYPE_ATA; // Default to ATA for now

    // TODO: If PCI discovery finds a USB drive, set type to DISK_TYPE_USB

    // --- Stage 1: Software Reset ---
    // Select the master drive
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    ATA_Wait400ns();

    // Perform a software reset
    i686_outb(ATA_PRIMARY_CONTROL, 0x04); // Set SRST (Software Reset)
    ATA_Wait400ns();
    i686_outb(ATA_PRIMARY_CONTROL, 0x00); // Clear SRST
    ATA_Wait400ns();

    // Wait for the drive to finish the reset.
    // Real hardware can take a long time to clear BSY after reset.
    // 10,000,000 iterations provides a more reliable window for physical disks.
    uint32_t timeout = 10000000; 
    while ((i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_BUSY) && --timeout);

    if (timeout == 0) {
        printf("DISK: Controller reset timeout.\n");
        return false;
    }

    // --- Stage 2: IDENTIFY command ---
    // Select the master drive again
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0 | (disk->id << 4));
    ATA_Wait400ns();

    // Zero out registers
    i686_outb(ATA_PRIMARY_SECTOR_COUNT, 0);
    i686_outb(ATA_PRIMARY_LBA_LOW, 0);
    i686_outb(ATA_PRIMARY_LBA_MID, 0);
    i686_outb(ATA_PRIMARY_LBA_HIGH, 0);
    ATA_Wait400ns();

    // Send IDENTIFY DEVICE command
    i686_outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY_DEVICE);
    ATA_Wait400ns();

    // Check status: 0xFF means floating bus (no controller/drive)
    uint8_t status = i686_inb(ATA_PRIMARY_CONTROL);

    // Check for drive presence
    if (status == 0 || status == 0xFF) {
        // Don't print an error here, it might just be an empty IDE channel
        return false;
    }

    // Poll for BSY to clear and DRQ or ERR to be set
    timeout = 10000000;
    while ((i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_BUSY) && --timeout);

    // Wait for DRQ (Data Request) to indicate the drive is ready to send identity data
    timeout = 10000000;
    while (!(i686_inb(ATA_PRIMARY_CONTROL) & (ATA_STATUS_DATA_REQUEST | ATA_STATUS_ERROR)) && --timeout);

    if (timeout == 0) return false;

    if (i686_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_ERROR) {
        printf("DISK: Drive %d not ready.\n", disk->id);
        return false;
    }

    // Read and discard the 512-byte identification data
    uint16_t identify_data[256];
    i686_insw(ATA_PRIMARY_DATA, identify_data, 256);

    printf("DISK: Initialized drive %d.\n", disk->id);
    return true;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t count, void* buffer) {
    if (disk->type == DISK_TYPE_USB) {
        // return USB_ReadSectors(disk, lba, count, buffer);
        printf("DISK: USB Read not yet implemented in protected mode!\n");
        return false;
    }

    // Wait until the drive is not busy
    uint32_t timeout = 0x0FFFFFFF;
    while ((i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_BUSY) && --timeout);

    // Select drive (Master) and send LBA bits 24-27
    // 0xE0 for master drive in LBA mode
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | (disk->id << 4) | ((lba >> 24) & 0x0F));
    ATA_Wait400ns();

    // Send the number of sectors to read
    i686_outb(ATA_PRIMARY_SECTOR_COUNT, count);

    // Send the LBA address (bits 0-23)
    i686_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    i686_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    i686_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    ATA_Wait400ns();

    // Send the READ SECTORS command
    i686_outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);

    uint16_t* target = (uint16_t*)buffer;

    for (int i = 0; i < count; i++) {
        // Poll until the drive is ready to transfer data (BSY clear, DRQ set)
        timeout = 0x0FFFFFFF;
        while (!((i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_DATA_REQUEST) || (i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_ERROR)) && --timeout);

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
    // Wait until the drive is not busy
    uint32_t timeout = 0x0FFFFFFF;
    while ((i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_BUSY) && --timeout);

    // Select drive (Master) and send LBA bits 24-27
    // 0xE0 for master drive in LBA mode
    i686_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | (disk->id << 4) | ((lba >> 24) & 0x0F));
    ATA_Wait400ns();

    // Send the number of sectors to write
    i686_outb(ATA_PRIMARY_SECTOR_COUNT, count);

    // Send the LBA address (bits 0-23)
    i686_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    i686_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    i686_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    ATA_Wait400ns();

    // Send the WRITE SECTORS command
    i686_outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);

    const uint16_t* source = (const uint16_t*)buffer;

    for (int i = 0; i < count; i++) {
        // Poll until the drive is ready to receive data (BSY clear, DRQ set)
        timeout = 0x0FFFFFFF;
        while (!((i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_DATA_REQUEST) || (i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_ERROR)) && --timeout);

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
    while ((i686_inb(ATA_PRIMARY_CONTROL) & ATA_STATUS_BUSY) && --timeout);

    return true;
}