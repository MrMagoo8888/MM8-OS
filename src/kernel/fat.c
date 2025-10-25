#include "fat.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"
#include "ctype.h"
#include "stddef.h"
#include "disk.h"

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

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;
    uint8_t VolumeLabel[11];
    uint8_t SystemId[8];

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
    bool IsModified; // Flag to track if the file's data buffer has been written to
    uint32_t DirectorySector; // Sector containing the directory entry for this file
    uint16_t DirectoryOffset; // Offset within the sector for this file's directory entry

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

static FAT_Data g_Data;
static uint8_t g_Fat[SECTOR_SIZE * 16]; // Max FAT size of 16 sectors (8KB)
static uint32_t g_DataSectionLba;


// This will hold the starting LBA of our FAT partition
static uint32_t g_PartitionOffset = 0;

bool FAT_ReadBootSector(DISK* disk)
{
    return DISK_ReadSectors(disk, g_PartitionOffset, 1, g_Data.BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK* disk)
{
    return DISK_ReadSectors(disk, g_PartitionOffset + g_Data.BS.BootSector.ReservedSectors, g_Data.BS.BootSector.SectorsPerFat, g_Fat);
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
        // We can assume no MBR and filesystem starts at 0, or fail. Let's fail for now.
        return false;
    }

    // Get the first partition entry
    MBR_PartitionEntry* partition = (MBR_PartitionEntry*)(mbr_buffer + 0x1BE);
    g_PartitionOffset = partition->start_lba;
    printf("FAT: Found partition starting at LBA %u\n", g_PartitionOffset);

    // read boot sector
    if (!FAT_ReadBootSector(disk))
    {
        printf("FAT: read boot sector failed\n");
        return false;
    }

    // read FAT
    if (!FAT_ReadFat(disk))
    {
        printf("FAT: read FAT failed\n");
        return false;
    }

    // Sanity check the boot sector
    if (g_Data.BS.BootSector.BytesPerSector == 0) {
        printf("FAT: Invalid bytes per sector in boot record.\n");
        return false;
    }

    // open root directory file
    uint32_t rootDirLba = g_PartitionOffset + g_Data.BS.BootSector.ReservedSectors + g_Data.BS.BootSector.SectorsPerFat * g_Data.BS.BootSector.FatCount;
    uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data.BS.BootSector.DirEntryCount;

    g_Data.RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data.RootDirectory.Public.IsDirectory = true;
    g_Data.RootDirectory.Public.Position = 0;
    g_Data.RootDirectory.Public.Size = sizeof(FAT_DirectoryEntry) * g_Data.BS.BootSector.DirEntryCount;
    g_Data.RootDirectory.Opened = true;
    g_Data.RootDirectory.FirstCluster = rootDirLba;
    g_Data.RootDirectory.CurrentCluster = rootDirLba;
    g_Data.RootDirectory.CurrentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, rootDirLba, 1, g_Data.RootDirectory.Buffer))
    {
        printf("FAT: read root directory failed\n");
        return false;
    }

    // calculate data section
    uint32_t rootDirSectors = (rootDirSize + g_Data.BS.BootSector.BytesPerSector - 1) / g_Data.BS.BootSector.BytesPerSector;
    g_DataSectionLba = rootDirLba + rootDirSectors;

    // reset opened files
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_Data.OpenedFiles[i].Opened = false;

    return true;
}

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    return g_DataSectionLba + (cluster - 2) * g_Data.BS.BootSector.SectorsPerCluster;
}

FAT_File* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry)
{
    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!g_Data.OpenedFiles[i].Opened)
            handle = i;
    }

    // out of handles
    if (handle < 0)
    {
        printf("FAT: out of file handles\n");
        return NULL;
    }

    // setup vars
    FAT_FileData* fd = &g_Data.OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;
    fd->IsModified = false;
    // DirectorySector and DirectoryOffset must be set by the caller (FAT_OpenInternal)

    if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        printf("FAT: open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
        return NULL;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FAT_NextCluster(uint32_t currentCluster)
{    
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
}

static void FAT_SetClusterValue(uint32_t cluster, uint16_t value) {
    uint32_t fatIndex = cluster * 3 / 2;
    if (cluster % 2 == 0) {
        *(uint16_t*)(g_Fat + fatIndex) = (*(uint16_t*)(g_Fat + fatIndex) & 0xF000) | (value & 0x0FFF);
    } else {
        *(uint16_t*)(g_Fat + fatIndex) = (*(uint16_t*)(g_Fat + fatIndex) & 0x000F) | (value << 4);
    }
}

static uint32_t FAT_FindAndAllocateFreeCluster(DISK* disk) {
    // Start search from cluster 2 (0 and 1 are reserved)
    // The total number of clusters can be derived from TotalSectors
    uint32_t total_clusters = (g_Data.BS.BootSector.TotalSectors - g_DataSectionLba) / g_Data.BS.BootSector.SectorsPerCluster;

    for (uint32_t i = 2; i < total_clusters; i++) {
        if (FAT_NextCluster(i) == 0x000) { // 0x000 indicates a free cluster
            // Mark cluster as end of chain
            FAT_SetClusterValue(i, 0xFFF);
            // Write the modified FAT sector back to disk
            uint32_t fat_sector = i * 3 / 2 / SECTOR_SIZE;
            DISK_WriteSectors(disk, g_PartitionOffset + g_Data.BS.BootSector.ReservedSectors + fat_sector, 1, g_Fat + fat_sector * SECTOR_SIZE);
            return i;
        }
    }
    return 0; // No free clusters
}

uint32_t FAT_Write(DISK* disk, FAT_File* file, uint32_t byteCount, const void* dataIn)
{
    FAT_FileData* fd = &g_Data.OpenedFiles[file->Handle];
    const uint8_t* u8DataIn = (const uint8_t*)dataIn;

    // For now, we don't support writing to directories
    if (fd->Public.IsDirectory) {
        return 0;
    }

    while (byteCount > 0)
    {
        uint32_t offset_in_buffer = fd->Public.Position % SECTOR_SIZE;
        uint32_t left_in_buffer = SECTOR_SIZE - offset_in_buffer;
        uint32_t take = min(byteCount, left_in_buffer);

        // Copy data to the file's sector buffer
        memcpy(fd->Buffer + offset_in_buffer, u8DataIn, take);
        fd->IsModified = true;

        u8DataIn += take;
        fd->Public.Position += take;
        byteCount -= take;

        // If the file size grew, update it
        if (fd->Public.Position > fd->Public.Size) {
            fd->Public.Size = fd->Public.Position;
        }

        // If the buffer is full, write it to disk and move to the next sector/cluster
        if (left_in_buffer == take)
        {
            // Write the current sector
            DISK_WriteSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer);

            if (++fd->CurrentSectorInCluster >= g_Data.BS.BootSector.SectorsPerCluster)
            {
                fd->CurrentSectorInCluster = 0;
                uint32_t nextCluster = FAT_NextCluster(fd->CurrentCluster);
                if (nextCluster >= 0xFF8) // End of chain
                {
                    // Allocate a new cluster
                    uint32_t newCluster = FAT_FindAndAllocateFreeCluster(disk);
                    if (newCluster == 0) {
                        printf("FAT: Out of disk space!\n");
                        return u8DataIn - (const uint8_t*)dataIn; // Return bytes written so far
                    }

                    // Link the old cluster to the new one
                    FAT_SetClusterValue(fd->CurrentCluster, newCluster);
                    // Write the modified FAT sector back to disk
                    uint32_t fat_sector = fd->CurrentCluster * 3 / 2 / SECTOR_SIZE;
                    DISK_WriteSectors(disk, g_PartitionOffset + g_Data.BS.BootSector.ReservedSectors + fat_sector, 1, g_Fat + fat_sector * SECTOR_SIZE);

                    nextCluster = newCluster;
                }
                fd->CurrentCluster = nextCluster;
            }

            // Read the next sector for continued writing
            if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
            {
                printf("FAT: read error during write!\n");
                break;
            }
        }
    }

    return u8DataIn - (const uint8_t*)dataIn;
}

void FAT_Flush(DISK* disk, FAT_File* file) {
    FAT_FileData* fd = &g_Data.OpenedFiles[file->Handle];

    if (fd->IsModified) {
        // Write the last modified sector to disk
        uint32_t lba = FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster;
        if (!DISK_WriteSectors(disk, lba, 1, fd->Buffer)) {
            printf("FAT: Failed to flush file handle %d\n", file->Handle);
        }

        // Update directory entry with new file size
        // We need to read the directory sector, modify it, and write it back.
        uint8_t dir_sector[SECTOR_SIZE];
        if (!DISK_ReadSectors(disk, fd->DirectorySector, 1, dir_sector)) {
            printf("FAT: Failed to read directory sector to update size for handle %d\n", file->Handle);
            return;
        }

        FAT_DirectoryEntry* entry = (FAT_DirectoryEntry*)(dir_sector + fd->DirectoryOffset);
        entry->Size = fd->Public.Size;
        // TODO: Update ModifiedTime and ModifiedDate

        if (!DISK_WriteSectors(disk, fd->DirectorySector, 1, dir_sector)) {
            printf("FAT: Failed to write directory sector to update size for handle %d\n", file->Handle);
        }

        fd->IsModified = false;
    }
}

uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut)
{
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) 
        ? &g_Data.RootDirectory 
        : &g_Data.OpenedFiles[file->Handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    if (!fd->Public.IsDirectory || (fd->Public.IsDirectory && fd->Public.Size != 0))
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
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
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
                if (++fd->CurrentSectorInCluster >= g_Data.BS.BootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFF8)
                {
                    fd->Public.Size = fd->Public.Position;
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

void FAT_Close(DISK* disk, FAT_File* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        // For the root directory, just reset its position.
        // No need to flush as we don't support writing to it yet.
        file->Position = 0;
        g_Data.RootDirectory.CurrentCluster = g_Data.RootDirectory.FirstCluster;
    }
    else
    {
        FAT_Flush(disk, file);
        g_Data.OpenedFiles[file->Handle].Opened = false;
    }
}

bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    char fatName[12];
    FAT_DirectoryEntry entry;

    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + strlen(name);

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (*ext == '.')
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);
    }

    while (FAT_ReadEntry(disk, file, &entry))
    {
        if (memcmp(fatName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }        
    }
    
    return false;
}

FAT_File* FAT_OpenInternal(DISK* disk, const char* path);
FAT_File* FAT_Open(DISK* disk, const char* path, FAT_OpenMode mode)
{
    FAT_DirectoryEntry entry;

    // Try to open the file
    FAT_File* file = FAT_OpenInternal(disk, path);

    if (file) // File exists
    {
        if (mode == FAT_OPEN_MODE_CREATE) {
            printf("FAT: File '%s' already exists.\n", path);
            FAT_Close(disk, file);
            return NULL;
        }
        return file;
    }
    else // File does not exist
    {
        if (mode == FAT_OPEN_MODE_CREATE) {
            // This is complex: we need to find the parent directory,
            // then create the file in it.
            // For now, let's just print a message.
            printf("FAT: File creation not fully implemented yet.\n");
            return NULL;
        } else {
            printf("FAT: File '%s' not found.\n", path);
            return NULL;
        }
    }
}

FAT_File* FAT_OpenInternal(DISK* disk, const char* path)
{
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        path++;

    FAT_File* current = &g_Data.RootDirectory.Public;

    while (*path) {
        // Extract the next path component
        const char* delim = strchr(path, '/');
        size_t len;
        if (delim) {
            len = delim - path;
        } else {
            len = strlen(path);
        }

        if (len >= MAX_PATH_SIZE) {
            // Path component is too long
            FAT_Close(disk, current);
            return NULL;
        }

        memcpy(name, path, len);
        name[len] = '\0';

        if (delim) {
            path = delim + 1;
        } else {
            path += len; // Move to the end of the string
        }
        
        // Before searching, get the directory's current LBA and position
        FAT_FileData* parent_fd = (current->Handle == ROOT_DIRECTORY_HANDLE) ? &g_Data.RootDirectory : &g_Data.OpenedFiles[current->Handle];
        uint32_t dir_lba = (current->Handle == ROOT_DIRECTORY_HANDLE) ? parent_fd->CurrentCluster : FAT_ClusterToLba(parent_fd->CurrentCluster) + parent_fd->CurrentSectorInCluster;
        uint32_t dir_offset = parent_fd->Public.Position;

        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry))
        {
            // If this is not the last component and it's not a directory, fail.
            if (*path != '\0' && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                printf("FAT: %s not a directory\n", name);
                return NULL;
            }

            // We found the entry. Close the directory we were searching in...
            FAT_Close(disk, current);
            // ...and open the new entry.
            current = FAT_OpenEntry(disk, &entry);
            if (current) {
                // Store the location of the directory entry we just found
                FAT_FileData* fd = &g_Data.OpenedFiles[current->Handle];
                fd->DirectorySector = dir_lba;
                fd->DirectoryOffset = dir_offset % SECTOR_SIZE;
            } else {
                // Failed to open the new entry (e.g., out of handles)
                return NULL;
            }
        }
        else
        {
            FAT_Close(disk, current);
            printf("FAT: %s not found\n", name);
            return NULL;
        }
    }

    return current;
}