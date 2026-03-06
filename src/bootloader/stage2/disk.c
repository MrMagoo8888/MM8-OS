#include "disk.h"
#include "x86.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"

typedef struct {
    uint8_t     bootable;
    uint8_t     start_head;
    uint8_t     start_sector : 6;
    uint16_t    start_cylinder : 10;
    uint8_t     system_id;
    uint8_t     end_head;
    uint8_t     end_sector : 6;
    uint16_t    end_cylinder : 10;
    uint32_t    start_lba;
    uint32_t    size_in_sectors;
} __attribute__((packed)) MBR_PartitionEntry;

static uint32_t g_PartitionOffset = 0;

bool DISK_Initialize(DISK* disk, uint8_t driveNumber)
{
    uint8_t driveType;
    uint16_t cylinders, sectors, heads;

    if (!x86_Disk_GetDriveParams(driveNumber, &driveType, &cylinders, &sectors, &heads)) {
        return false;
    }

    // If we are on a hard disk, find the partition offset from the MBR
    g_PartitionOffset = 0;
    if (driveNumber >= 0x80) {
        uint8_t mbr_buffer[512];
        // Temporarily read sector 0 without an offset to find the partition
        if (x86_Disk_Read(driveNumber, 0, 1, 0, 1, mbr_buffer) && *(uint16_t*)(mbr_buffer + 0x1FE) == 0xAA55) {
            MBR_PartitionEntry* partition = (MBR_PartitionEntry*)(mbr_buffer + 0x1BE);
            g_PartitionOffset = partition->start_lba;
        }
    }

    disk->id = driveNumber;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->sectors = sectors;

    return true;
}

void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorOut, uint16_t* headOut)
{
    // sector = (LBA % sectors per track + 1)
    *sectorOut = lba % disk->sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (lba / disk->sectors) / disk->heads;

    // head = (LBA / sectors per track) % heads
    *headOut = (lba / disk->sectors) % disk->heads;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void* dataOut)
{
    uint16_t cylinder, sector, head;

    DISK_LBA2CHS(disk, lba + g_PartitionOffset, &cylinder, &sector, &head);

    for (int i = 0; i < 3; i++)
    {
        if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, dataOut))
            return true;

        x86_Disk_Reset(disk->id);
    }

    return false;
}