#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "disk.h"

typedef struct
{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    union {
        // FAT12/16 Extended Boot Record
        struct {
            uint8_t DriveNumber;
            uint8_t _Reserved;
            uint8_t Signature;
            uint32_t VolumeId;
            uint8_t VolumeLabel[11];
            uint8_t SystemId[8];
        } fat16;
        // FAT32 Extended Boot Record
        struct {
            uint32_t SectorsPerFat32;
            uint16_t Flags;
            uint16_t FatVersion;
            uint32_t RootCluster;
            uint16_t FSInfoSector;
            uint16_t BackupBootSector;
            uint8_t _Reserved[12];
            uint8_t DriveNumber;
            uint8_t _Reserved2;
            uint8_t Signature;
            uint32_t VolumeId;
            uint8_t VolumeLabel[11];
            uint8_t SystemId[8];
        } __attribute__((packed)) fat32;
    } Ebr;

} __attribute__((packed)) FAT_BootSector;

typedef struct
{
    int Handle;
    bool IsDirectory;
    uint32_t Position;
    uint32_t Size;
} FAT_File;

// File open modes
typedef enum {
    FAT_OPEN_MODE_READ,
    FAT_OPEN_MODE_WRITE,
    FAT_OPEN_MODE_CREATE,
} FAT_OpenMode;

typedef struct
{
    uint8_t Buffer[512]; // SECTOR_SIZE
    FAT_File Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;
    bool IsModified;

} FAT_FileData;

typedef struct
{
    union
    {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[512]; // SECTOR_SIZE
    } BS;

    FAT_FileData RootDirectory;
    FAT_FileData OpenedFiles[10]; // MAX_FILE_HANDLES
} FAT_Data;

typedef struct
{
    char Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) FAT_DirectoryEntry;

enum FAT_Attributes
{
    FAT_ATTRIBUTE_READ_ONLY = 0x01,
    FAT_ATTRIBUTE_HIDDEN = 0x02,
    FAT_ATTRIBUTE_SYSTEM = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID = 0x08,
    FAT_ATTRIBUTE_DIRECTORY = 0x10,
    FAT_ATTRIBUTE_ARCHIVE = 0x20,
    FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

bool FAT_Initialize(DISK* disk);
FAT_File* FAT_Open(DISK* disk, const char* path, FAT_OpenMode mode);
uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut);
uint32_t FAT_Write(DISK* disk, FAT_File* file, uint32_t byteCount, const void* dataIn);
void FAT_Close(DISK* disk, FAT_File* file);
bool FAT_Seek(DISK* disk, FAT_File* file, uint32_t offset);
bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut);