/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "sdcard.h"

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    // already initialized in init() procedure
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    DWORD sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    if(SDCARD_ReadBegin(sector) < 0) {
        return RES_ERROR;
    }

    while(count > 0) {
        if(SDCARD_ReadData(buff) < 0) {
            return RES_ERROR;
        }
        buff += 512;
        count--;
    }

    if(SDCARD_ReadEnd() < 0) {
        return RES_ERROR;
    }

    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    if(SDCARD_WriteBegin(sector) < 0) {
        return RES_ERROR;
    }

    while(count > 0) {
        if(SDCARD_WriteData(buff) < 0) {
            return RES_ERROR;
        }

        buff += 512;
        count--;
    }

    if(SDCARD_WriteEnd() < 0) {
        return RES_ERROR;
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    if(cmd == CTRL_SYNC) {
        return RES_OK;
    } else {
        // should never be called
        return RES_ERROR;
    }
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */

DWORD get_fattime(void)
{
    extern RTC_HandleTypeDef hrtc;
    RTC_TimeTypeDef time;
    HAL_StatusTypeDef res = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    if (res != HAL_OK) return 0;
    RTC_DateTypeDef date;
    res = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if (res != HAL_OK) return 0;

    return (DWORD)(date.Year + 20) << 25 |
           (DWORD)(date.Month + 1) << 21 |
           (DWORD)date.Date << 16 |
           (DWORD)time.Hours << 11 |
           (DWORD)time.Minutes << 5 |
           (DWORD)time.Seconds >> 1;
}

