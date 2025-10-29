#include "fat.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"
#include "ctype.h"
#include "../bootloader/stage2/memdefs.h"
#include "stddef.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))

#define SECTOR_SIZE             512
#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1

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

typedef struct
{
    uint8_t Buffer[SECTOR_SIZE];
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
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;
    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

} FAT_Data;

// --- FAT Type Detection ---
typedef enum {
    FAT_TYPE_UNKNOWN,
    FAT_TYPE_FAT12,
    FAT_TYPE_FAT16,
    FAT_TYPE_FAT32,
} FAT_Type;

static FAT_Type g_FatType = FAT_TYPE_UNKNOWN;
static FAT_Data* g_Data;
static uint8_t g_Fat[SECTOR_SIZE * 16]; // Max FAT size of 16 sectors (8KB)
static uint32_t g_DataSectionLba;

// This will hold the starting LBA of our FAT partition
static uint32_t g_PartitionOffset = 0;

bool FAT_ReadBootSector(DISK* disk)
{
    return DISK_ReadSectors(disk, g_PartitionOffset, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK* disk)
{
    // Read only as many sectors as we have space for in our buffer.
    uint32_t sectorsToRead = min(g_Data->BS.BootSector.SectorsPerFat, sizeof(g_Fat) / SECTOR_SIZE);
    return DISK_ReadSectors(disk, g_PartitionOffset + g_Data->BS.BootSector.ReservedSectors, sectorsToRead, g_Fat);
}

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    // Root directory on FAT12/16 is a special case, it's not in the data section
    if (g_FatType != FAT_TYPE_FAT32 && cluster < 2) {
        return cluster;
    }
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

bool FAT_Initialize(DISK* disk)
{
    // --- Read MBR to find the partition ---
    uint8_t mbr_buffer[SECTOR_SIZE];
    if (!DISK_ReadSectors(disk, 0, 1, mbr_buffer)) {
        printf("FAT: Failed to read MBR.\n");
        return false;
    }

    // Check for MBR signature
    if (*(uint16_t*)(mbr_buffer + 0x1FE) != 0xAA55) {
        printf("FAT: Invalid MBR signature.\n");
        return false;
    }

    // Get the first partition entry
    MBR_PartitionEntry* partition = (MBR_PartitionEntry*)(mbr_buffer + 0x1BE);
    g_PartitionOffset = partition->start_lba;
    printf("FAT: Found partition starting at LBA %u\n", g_PartitionOffset);

    // Allocate memory for the FAT_Data structure
    g_Data = (FAT_Data*)MEMORY_FAT_ADDR;

    // read boot sector
    if (!FAT_ReadBootSector(disk))
    {
        printf("FAT: read boot sector failed\n");
        return false;
    }

    // Sanity check the boot sector
    if (g_Data->BS.BootSector.BytesPerSector == 0) {
        printf("FAT: Invalid bytes per sector in boot record.\n");
        return false;
    }

    // --- Determine FAT Type ---
    uint32_t rootDirSectors;
    uint32_t dataSectors;
    uint32_t clusterCount;

    if (g_Data->BS.BootSector.SectorsPerFat == 0) {
        // This indicates a FAT32 volume
        g_FatType = FAT_TYPE_FAT32;
    } else {
        // Calculate cluster count for FAT12/16
        rootDirSectors = ((g_Data->BS.BootSector.DirEntryCount * 32) + (g_Data->BS.BootSector.BytesPerSector - 1)) / g_Data->BS.BootSector.BytesPerSector;
        dataSectors = g_Data->BS.BootSector.TotalSectors - (g_Data->BS.BootSector.ReservedSectors + (g_Data->BS.BootSector.FatCount * g_Data->BS.BootSector.SectorsPerFat) + rootDirSectors);
        clusterCount = dataSectors / g_Data->BS.BootSector.SectorsPerCluster;
        if (clusterCount < 4085) {
            g_FatType = FAT_TYPE_FAT12;
        } else if (clusterCount < 65525) {
            g_FatType = FAT_TYPE_FAT16;
        } else {
            // Some FAT16 volumes can have more clusters, but this is a rare case.
            // For our purposes, we'll assume it's FAT32 if it's this large.
            g_FatType = FAT_TYPE_FAT32;
        }
    }

    // --- Initialize based on FAT type ---
    if (g_FatType == FAT_TYPE_FAT32) {
        printf("FAT: Detected FAT32 filesystem\n");
        // For FAT32, the SectorsPerFat comes from the 32-bit field.
        g_Data->BS.BootSector.SectorsPerFat = g_Data->BS.BootSector.Ebr.fat32.SectorsPerFat32;
        g_DataSectionLba = g_PartitionOffset + g_Data->BS.BootSector.ReservedSectors + (g_Data->BS.BootSector.FatCount * g_Data->BS.BootSector.SectorsPerFat);
        
        uint32_t rootCluster = g_Data->BS.BootSector.Ebr.fat32.RootCluster;
        g_Data->RootDirectory.FirstCluster = rootCluster;
        g_Data->RootDirectory.CurrentCluster = rootCluster;
        g_Data->RootDirectory.Public.Size = 0; // Size is dynamic for FAT32 root directory

        if (!DISK_ReadSectors(disk, FAT_ClusterToLba(rootCluster), 1, g_Data->RootDirectory.Buffer)) {
            printf("FAT: read root directory failed\n");
            return false;
        }

    } else if (g_FatType == FAT_TYPE_FAT12) {
        printf("FAT: Detected FAT12 filesystem\n");
        uint32_t rootDirLba = g_PartitionOffset + g_Data->BS.BootSector.ReservedSectors + (g_Data->BS.BootSector.FatCount * g_Data->BS.BootSector.SectorsPerFat);
        rootDirSectors = ((g_Data->BS.BootSector.DirEntryCount * sizeof(FAT_DirectoryEntry)) + (g_Data->BS.BootSector.BytesPerSector - 1)) / g_Data->BS.BootSector.BytesPerSector;
        g_DataSectionLba = rootDirLba + rootDirSectors;
        
        g_Data->RootDirectory.FirstCluster = rootDirLba; // Special case for FAT12 root
        g_Data->RootDirectory.CurrentCluster = rootDirLba;
        g_Data->RootDirectory.Public.Size = g_Data->BS.BootSector.DirEntryCount * sizeof(FAT_DirectoryEntry);

        if (!DISK_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory.Buffer)) {
            printf("FAT: read root directory failed\n");
            return false;
        }
    } else {
        printf("FAT: Detected FAT16 or unsupported filesystem\n");
        return false;
    }

    // Now that we know the correct SectorsPerFat (especially for FAT32), read the FAT
    if (!FAT_ReadFat(disk))
    {
        printf("FAT: read FAT failed\n");
        return false;
    }

    // Common initialization for the root directory handle
    g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.Public.IsDirectory = true;
    g_Data->RootDirectory.Public.Position = 0;
    g_Data->RootDirectory.Opened = true;
    g_Data->RootDirectory.CurrentSectorInCluster = 0;

    // reset opened files
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_Data->OpenedFiles[i].Opened = false;

    return true;
}

FAT_File* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry)
{
    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!g_Data->OpenedFiles[i].Opened)
            handle = i;
    }

    // out of handles
    if (handle < 0)
    {
        printf("FAT: out of file handles\n");
        return NULL;
    }

    // setup vars
    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow | ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;
    fd->IsModified = false;

    // If the file has content (FirstCluster is not 0), read its first sector.
    if (fd->FirstCluster != 0)
    {
        if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
        {
            printf("FAT: open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
            return NULL;
        }
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FAT_NextCluster(uint32_t currentCluster)
{    
    switch (g_FatType) {
        case FAT_TYPE_FAT12:
        {
            uint32_t fatIndex = currentCluster * 3 / 2;
            uint16_t val = *(uint16_t*)(g_Fat + fatIndex);
            if (currentCluster % 2 == 0)
                return val & 0x0FFF;
            else
                return val >> 4;
        }
        case FAT_TYPE_FAT32:
        {
            uint32_t* fat_table = (uint32_t*)g_Fat;
            return fat_table[currentCluster] & 0x0FFFFFFF; // Mask out top 4 bits
        }
        default:
            return 0x0FFFFFFF; // End of chain for unsupported types
    }
}

static void FAT_SetClusterValue(uint32_t cluster, uint32_t value) {
    switch (g_FatType) {
        case FAT_TYPE_FAT12:
        {
            uint32_t fatIndex = cluster * 3 / 2;
            if (cluster % 2 == 0) {
                *(uint16_t*)(g_Fat + fatIndex) = (*(uint16_t*)(g_Fat + fatIndex) & 0xF000) | (value & 0x0FFF);
            } else {
                *(uint16_t*)(g_Fat + fatIndex) = (*(uint16_t*)(g_Fat + fatIndex) & 0x000F) | (value << 4);
            }
            break;
        }
        case FAT_TYPE_FAT32:
        {
            uint32_t* fat_table = (uint32_t*)g_Fat;
            fat_table[cluster] = (fat_table[cluster] & 0xF0000000) | (value & 0x0FFFFFFF);
            break;
        }
    }
}

static uint32_t FAT_FindAndAllocateFreeCluster(DISK* disk) {
    // Start search from cluster 2 (0 and 1 are reserved)
    uint32_t total_clusters = (g_Data->BS.BootSector.TotalSectors - g_DataSectionLba) / g_Data->BS.BootSector.SectorsPerCluster;

    for (uint32_t i = 2; i < total_clusters; i++) {
        if (FAT_NextCluster(i) == 0x000) { // 0x000 indicates a free cluster
            // Mark cluster as end of chain
            if (g_FatType == FAT_TYPE_FAT32) {
                FAT_SetClusterValue(i, 0x0FFFFFFF);
            } else {
                FAT_SetClusterValue(i, 0xFFF);
            }

            // Write the modified FAT sector back to disk
            uint32_t fat_offset = (g_FatType == FAT_TYPE_FAT12) ? (i * 3 / 2) : (i * 4);
            uint32_t fat_sector = fat_offset / SECTOR_SIZE;
            DISK_WriteSectors(disk, g_PartitionOffset + g_Data->BS.BootSector.ReservedSectors + fat_sector, 1, g_Fat + (fat_sector * SECTOR_SIZE));
            return i;
        }
    }
    printf("FAT: Out of disk space!\n");
    return 0; // No free clusters
}

void FAT_Flush(DISK* disk, FAT_File* file) {
    FAT_FileData* fd = &g_Data->OpenedFiles[file->Handle];

    if (fd->IsModified) {
        // Write the last modified sector to disk
        uint32_t lba = FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster;
        if (!DISK_WriteSectors(disk, lba, 1, fd->Buffer)) {
            printf("FAT: Failed to flush file handle %d\n", file->Handle);
        }
        fd->IsModified = false;
    }
}

uint32_t FAT_Write(DISK* disk, FAT_File* file, uint32_t byteCount, const void* dataIn)
{
    FAT_FileData* fd = &g_Data->OpenedFiles[file->Handle];
    const uint8_t* u8DataIn = (const uint8_t*)dataIn;

    if (fd->Public.IsDirectory) {
        return 0;
    }

    while (byteCount > 0)
    {
        uint32_t offset_in_buffer = fd->Public.Position % SECTOR_SIZE;
        uint32_t left_in_buffer = SECTOR_SIZE - offset_in_buffer;
        uint32_t take = min(byteCount, left_in_buffer);

        memcpy(fd->Buffer + offset_in_buffer, u8DataIn, take);
        fd->IsModified = true;

        u8DataIn += take;
        fd->Public.Position += take;
        byteCount -= take;

        if (fd->Public.Position > fd->Public.Size) {
            fd->Public.Size = fd->Public.Position;
        }

        if (left_in_buffer == take)
        {
            DISK_WriteSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer);

            if (++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster)
            {
                fd->CurrentSectorInCluster = 0;
                uint32_t nextCluster = FAT_NextCluster(fd->CurrentCluster);
                
                bool isEndOfChain = (g_FatType == FAT_TYPE_FAT32)
                                    ? (nextCluster >= 0x0FFFFFF8)
                                    : (nextCluster >= 0xFF8);

                if (isEndOfChain)
                {
                    uint32_t newCluster = FAT_FindAndAllocateFreeCluster(disk);
                    if (newCluster == 0) {
                        return u8DataIn - (const uint8_t*)dataIn;
                    }

                    FAT_SetClusterValue(fd->CurrentCluster, newCluster);
                    uint32_t fat_offset = (g_FatType == FAT_TYPE_FAT12) ? (fd->CurrentCluster * 3 / 2) : (fd->CurrentCluster * 4);
                    uint32_t fat_sector = fat_offset / SECTOR_SIZE;
                    DISK_WriteSectors(disk, g_PartitionOffset + g_Data->BS.BootSector.ReservedSectors + fat_sector, 1, g_Fat + (fat_sector * SECTOR_SIZE));

                    nextCluster = newCluster;
                }
                fd->CurrentCluster = nextCluster;
            }

            if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
            {
                printf("FAT: read error during write!\n");
                break;
            }
        }
    }

    return u8DataIn - (const uint8_t*)dataIn;
}

uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut)
{
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) 
        ? &g_Data->RootDirectory 
        : &g_Data->OpenedFiles[file->Handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    if (file->Handle != ROOT_DIRECTORY_HANDLE || g_FatType == FAT_TYPE_FAT12)
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        if (leftInBuffer == take)
        {
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE && g_FatType == FAT_TYPE_FAT12)
            {
                ++fd->CurrentCluster;
                if (!DISK_ReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\n");
                    break;
                }
            }
            else
            {
                if (++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
                }

                bool isEndOfChain = (g_FatType == FAT_TYPE_FAT32)
                                    ? (fd->CurrentCluster >= 0x0FFFFFF8)
                                    : (fd->CurrentCluster >= 0xFF8);
                if (isEndOfChain)
                {
                    break;
                }

                if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\n");
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8_t*)dataOut;
}

bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry)
{
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

bool FAT_Seek(DISK* disk, FAT_File* file, uint32_t offset)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE) {
        // Seeking in root directory is simpler
        file->Position = offset;
        return true;
    }

    FAT_FileData* fd = &g_Data->OpenedFiles[file->Handle];

    // Rewind to the beginning
    fd->Public.Position = 0;
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (offset == 0) {
        // If seeking to the beginning, just read the first sector
        DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer);
        return true;
    }

    // To seek, we just read and discard data until we reach the desired position.
    // This is inefficient but simple and effective for now.
    uint8_t temp_buffer[SECTOR_SIZE];
    uint32_t remaining = offset;
    while (remaining > 0) {
        uint32_t to_read = min(remaining, SECTOR_SIZE);
        uint32_t bytes_read = FAT_Read(disk, file, to_read, temp_buffer);
        if (bytes_read != to_read) {
            // Couldn't read enough bytes, seek failed
            return false;
        }
        remaining -= bytes_read;
    }
    return true;
}

void FAT_Close(DISK* disk, FAT_File* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
    }
    else
    {
        FAT_FileData* fd = &g_Data->OpenedFiles[file->Handle];
        if (fd->Opened) {
            FAT_Flush(disk, file);

            if (fd->IsModified) {
                // Rewind root directory to search from the beginning
                g_Data->RootDirectory.Public.Position = 0;
                FAT_DirectoryEntry entry;

                while(FAT_ReadEntry(disk, &g_Data->RootDirectory.Public, &entry)) {
                    if ((entry.FirstClusterLow | ((uint32_t)entry.FirstClusterHigh << 16)) == fd->FirstCluster) {
                        // Found the entry. Update its size.
                        entry.Size = fd->Public.Size;

                        // Calculate the exact position of the entry on disk
                        uint32_t entryAbsPosition = g_Data->RootDirectory.Public.Position - sizeof(FAT_DirectoryEntry);
                        uint32_t entrySector = entryAbsPosition / SECTOR_SIZE;
                        uint32_t entryOffsetInSector = entryAbsPosition % SECTOR_SIZE;

                        // Read the correct sector, modify it, and write it back.
                        uint8_t sectorBuffer[SECTOR_SIZE];
                        DISK_ReadSectors(disk, FAT_ClusterToLba(g_Data->RootDirectory.FirstCluster) + entrySector, 1, sectorBuffer);
                        memcpy(sectorBuffer + entryOffsetInSector, &entry, sizeof(FAT_DirectoryEntry));
                        DISK_WriteSectors(disk, FAT_ClusterToLba(g_Data->RootDirectory.FirstCluster) + entrySector, 1, sectorBuffer);
                        break;
                    }
                }
            }
        }
        fd->Opened = false;
    }
}

bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    file->Position = 0;

    char fatName[12];
    FAT_DirectoryEntry entry;

    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strrchr(name, '.');
    int name_len = (ext) ? (ext - name) : strlen(name);
    
    if (name_len > 8) name_len = 8;
    for (int i = 0; i < name_len; i++)
        fatName[i] = toupper(name[i]);

    if (ext)
    {
        ext++; // Skip the dot
        int ext_len = strlen(ext);
        if (ext_len > 3) ext_len = 3;
        for (int i = 0; i < ext_len; i++)
            fatName[i + 8] = toupper(ext[i]);
    }

    while (FAT_ReadEntry(disk, file, &entry))
    {
        if (entry.Name[0] == 0x00) break; // End of directory
        if (entry.Name[0] == 0xE5) continue; // Deleted entry
        if (memcmp(fatName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }        
    }
    
    return false;
}

FAT_File* FAT_Open(DISK* disk, const char* path, FAT_OpenMode mode)
{
    if (path[0] == '/')
        path++;

    FAT_File* current = &g_Data->RootDirectory.Public;
    current->Position = 0;

    FAT_DirectoryEntry existing_entry;
    if (FAT_FindFile(disk, current, path, &existing_entry)) {
        return FAT_OpenEntry(disk, &existing_entry);
    }

    if (mode != FAT_OPEN_MODE_CREATE) {
        printf("FAT: %s not found\n", path);
        return NULL;
    }

    if (strchr(path, '/')) {
        printf("FAT: Creation only supported in root directory for now.\n");
        return NULL;
    }

    current->Position = 0;
    FAT_DirectoryEntry new_entry;
    while (FAT_ReadEntry(disk, current, &new_entry)) {
        if (new_entry.Name[0] == 0x00 || new_entry.Name[0] == 0xE5) {
            uint32_t entry_offset_in_sector = (current->Position - sizeof(FAT_DirectoryEntry)) % SECTOR_SIZE;

            memset(&new_entry, 0, sizeof(new_entry));
            memset(new_entry.Name, ' ', 11);
            
            const char* ext = strrchr(path, '.');
            int name_len = (ext) ? (ext - path) : strlen(path);
            if (name_len > 8) name_len = 8;
            for (int i = 0; i < name_len; i++) new_entry.Name[i] = toupper(path[i]);
            
            if (ext) {
                ext++;
                int ext_len = strlen(ext);
                if (ext_len > 3) ext_len = 3;
                for (int i = 0; i < ext_len; i++) new_entry.Name[8 + i] = toupper(ext[i]);
            }

            new_entry.Attributes = FAT_ATTRIBUTE_ARCHIVE;
            new_entry.FirstClusterLow = 0;
            new_entry.Size = 0;

            memcpy(g_Data->RootDirectory.Buffer + entry_offset_in_sector, &new_entry, sizeof(FAT_DirectoryEntry));
            uint32_t lba = (g_FatType == FAT_TYPE_FAT12)
                ? g_Data->RootDirectory.CurrentCluster
                : FAT_ClusterToLba(g_Data->RootDirectory.CurrentCluster) + g_Data->RootDirectory.CurrentSectorInCluster;
            DISK_WriteSectors(disk, lba, 1, g_Data->RootDirectory.Buffer);

            return FAT_OpenEntry(disk, &new_entry);
        }
    }
    printf("FAT: No free space in root directory.\n");
    return NULL;
}
