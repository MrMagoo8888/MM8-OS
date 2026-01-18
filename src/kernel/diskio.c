/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for MM8-OS             (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */
#include "disk.h"       /* MM8-OS Disk Driver */

/* Global disk instance defined in main.c */
extern DISK g_Disk;

#define DEV_HDD		0


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv != DEV_HDD) return STA_NOINIT;
    
    // MM8-OS disk driver doesn't expose status check yet.
    // We assume it is initialized if we are here, or main.c handles it.
	return 0; // RES_OK
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv != DEV_HDD) return STA_NOINIT;

    // Initialize the hard disk (0x80)
    if (DISK_Initialize(&g_Disk, 0x80)) {
        return 0; // RES_OK
    }

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if (pdrv != DEV_HDD) return RES_PARERR;

    // DISK_ReadSectors takes uint8_t for count, so we chunk it if necessary.
    while (count > 0) {
        uint8_t to_read = (count > 128) ? 128 : (uint8_t)count;
        if (!DISK_ReadSectors(&g_Disk, sector, to_read, buff)) {
            return RES_ERROR;
        }
        sector += to_read;
        buff += to_read * 512; // Assuming 512 byte sectors
        count -= to_read;
    }

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	if (pdrv != DEV_HDD) return RES_PARERR;

    // DISK_WriteSectors takes uint8_t for count, so we chunk it.
    while (count > 0) {
        uint8_t to_read = (count > 128) ? 128 : (uint8_t)count;
        if (!DISK_WriteSectors(&g_Disk, sector, to_read, buff)) {
            return RES_ERROR;
        }
        sector += to_read;
        buff += to_read * 512;
        count -= to_read;
    }

	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    if (pdrv != DEV_HDD) return RES_PARERR;

	DRESULT res = RES_ERROR;

	switch (cmd) {
	case CTRL_SYNC :
        // Flush write cache if implemented. For simple PIO, writes are usually immediate.
		res = RES_OK;
        break;

    case GET_SECTOR_COUNT :
        // Return total sectors. Hardcoded dummy value for now (e.g., 512MB).
        // You should implement a function in disk.c to read drive capacity.
        *(LBA_t*)buff = 1024 * 1024; 
        res = RES_OK;
        break;

    case GET_SECTOR_SIZE :
        *(WORD*)buff = 512;
        res = RES_OK;
        break;

    case GET_BLOCK_SIZE :
        *(DWORD*)buff = 1; // Block size in sectors
        res = RES_OK;
        break;

    default:
        res = RES_PARERR;
        break;
	}

	return res;
}

// FatFs requires a time function.
DWORD get_fattime (void)
{
    // Return a fixed time: 2023-01-01 00:00:00
    // Format: Year(7) | Month(4) | Day(5) | Hour(5) | Min(6) | Sec(5)
    return ((DWORD)(2023 - 1980) << 25) | 
           ((DWORD)1 << 21) | 
           ((DWORD)1 << 16) | 
           ((DWORD)0 << 11) | 
           ((DWORD)0 << 5) | 
           ((DWORD)0 >> 1);
}
