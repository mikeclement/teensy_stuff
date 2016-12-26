/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
//#include "usbdisk.h"	/* Example: USB drive control */
//#include "atadrive.h"	/* Example: ATA drive control */
#include "sdcard.h"		/* Example: MMC/SDC contorl */
//#include "term_io.h"	// Debug support

/* Definitions of physical drive number for each media */
#define ATA		0
#define MMC		1
#define USB		2


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	DSTATUS				stat;
	uint32_t			result;

//	xprintf("\n\rdisk_initialize, pdrv=%d.", pdrv);		// debug
	switch (pdrv)
	{
		case 0 :					// first (only) drive is SD card
		result = SDInit();			// returns 0 if initialize worked properly
//		xprintf("\n\rIn disk_initialize(), SDInit returns %d.", result);
		// translate the reslut code here
		if	    (result == SDCARD_NO_DETECT)  stat = STA_NODISK;
		else if (result == SDCARD_TIMEOUT)  stat = STA_NOINIT;
		else if (result == SDCARD_NOT_REG)  stat = STA_NODISK;		// not strictly true, but...
		else								stat = 0;
		return stat;

		default:
		stat = STA_NODISK;			// report no such disk
		break;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	switch (pdrv)
	{
		case 0 :						// first (only) drive is SD card
		result = SDStatus();
		// translate the reslut code here
		if (result == SDCARD_OK)  stat = 0;
		else  stat = STA_NODISK;				// incomplete; need to allow for write-protect
		return stat;

		default:
		break;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	DRESULT			res;
	int32_t			result;

	switch (pdrv)
	{
		case 0 :				// first (only) drive is SD card
		// translate the arguments here
		res = RES_OK;			// assume this works
		while (count)
		{
			result = SDReadBlock(sector, buff);
			if (result != SDCARD_OK)
			{
				res = RES_ERROR;
				break;
			}
			sector++;
			count--;
			buff = buff + 512;		// SD card library uses sector size of 512; FatFS better, also!
		}
		// translate the reslut code here
		return res;

		default:
		break;
	}
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	DRESULT			res;
	int32_t			result;

	switch (pdrv)
	{
		case 0 :				// first (only) drive is SD card
		res = RES_OK;			// assume this works
		// translate the arguments here
		while (count)
		{
			result = SDWriteBlock(sector, (uint8_t *)buff);
//			xprintf("\n\rIn disk_write(), SDWriteBlock returns %d.", result);
			if (result != SDCARD_OK)
			{
				res = RES_ERROR;
				break;
			}
			sector++;
			count--;
			buff = buff + 512;			// SD card library uses sector size of 512; FatFS better, also!
		}
		// translate the reslut code here
		return res;

		default:
		break;
	}
	return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	switch (pdrv)
	{
		case 0 :				// first (only) drive is SD card
		res = RES_OK;
//		xprintf("\n\rdisk_ioctl called, pdrv=%d, cmd=%d.", pdrv, cmd);
		switch (cmd)
		{
			case CTRL_SYNC:
			res = RES_OK;				// write-cache flushing is done automatically for SD card
			break;

			case GET_SECTOR_COUNT :	  // Get number of sectors on the disk (DWORD)
			res = RES_PARERR;
			break;

			case GET_SECTOR_SIZE :	  // Get R/W sector size (WORD) 
			*(WORD*)buff = 512;
			res = RES_OK;
			break;

			case GET_BLOCK_SIZE :	    // Get erase block size in unit of sector (DWORD)
			*(DWORD*)buff = 1;
			res = RES_OK;

			default:
			res = RES_PARERR;			// no idea what he wants, throw an error
			break;
		}
//		xprintf("\n\rdisk_ioctl result is %d.", res);

		// post-process here
		return res;

		default:
		break;
	}
	return RES_PARERR;
}
#endif


