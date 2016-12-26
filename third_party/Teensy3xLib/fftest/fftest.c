/*
 *  fftest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will set up a UART as a console port.  It will then
 *  initialize SPI0 for 400 kHz SPI comms to an SD card.  It will then
 *  perform a series of FatFS operations on the card.
 *
 *  To use this program, connect your SD card to the Teensy 3.1.  Note
 *  that the pins on the Teensy are based on the printed label on
 *  the Teensy PWB!
 *
 *  Teensy      SD card     Signal
 *  (label)
 *  ------		-------		---------------------------
 *     2		   1		chip select (PD0 on Teensy)
 *    11		   2		SD data in (PC6 on Teensy)
 *   GND		 3 & 6		Vss
 *  3.3V		   4		Vdd (be sure to use 3.3 V!
 *    13		   5		SCLK (PC5 on Teensy)
 *    12		   7		SD data out (PC7 on Teensy)
 */

#include  <stdio.h>
#include  <string.h>
#include  <stdint.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "sdcard.h"
#include  "uart.h"
#include  "spi.h"
#include  "termio.h"
#include  "ff.h"
#include  "diskio.h"

#define  MAX_BUFF_LEN		256

const char				hello[] = "fftest\r\n";
char					buff[MAX_BUFF_LEN+1];
uint32_t				sckfreqkhz;
uint8_t					readdata[512];
uint8_t					writedata[512];

FATFS					FatFs;   /* Work area (file system object) for logical drive */

FIL						fil;
char					filename[] = "test__0.txt";
char					readname[] = "fftest.c";
FRESULT					fres;

#define		CS_HIGH		GPIOD_PSOR=(1<<0)
#define		CS_LOW		GPIOD_PCOR=(1<<0)



/*
 *  Local functions
 */
static  void			select(void);
static  void			deselect(void);
static  char			xchg(char  c);
static void				type_file(char  *fn);
static FRESULT			scan_files (char  *path);


#define		MY_SPI		0		/* SPI channel to use (0 or 1) */

int  main(void)
{
	uint32_t				finalclk;

	UARTInit(TERM_UART, TERM_BAUD);	// open a console port
	xprintf(hello);

	PORTD_PCR0 = PORT_PCR_MUX(0x1); // CS is on PD0 (pin 2), config as GPIO (alt = 1)
	GPIOD_PDDR = (1<<0);			// make this an output pin
	CS_HIGH;						// and deselect the SD card

	sckfreqkhz = 400;				// use slow initial SPI clock
	finalclk = SPIInit(MY_SPI, sckfreqkhz, 8);
	xprintf("\n\rSPI connected at %d kHz.", finalclk);
	
	SDRegisterSPI(select, xchg, deselect);	// register our SPI functions with the SD library
	SDInit();						// now init the SD interface and card

	sckfreqkhz = 16000;				// switch to fast SPI clock
	finalclk = SPIInit(MY_SPI, sckfreqkhz, 8);
	xprintf("\n\rSPI connected at %d kHz.", finalclk);

	xprintf("\n\rOpening %s for trial write...", filename);
    f_mount(&FatFs, "", 0);
	fres = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (fres == FR_OK)
	{
		f_puts("Hello, world!\n\r", &fil);
		f_close(&fil);
		xprintf("done.");
		
		xprintf("\n\r\n\rDirectory listing of 0:...\n\r");
		scan_files("0:");

		xprintf("\n\r\n\rPartial contents of %s...\n\r", readname);
		type_file(readname);
	}
	else
	{
		xprintf("\n\rError: f_open returns %d", fres);
	}
	xprintf("\n\r");

	while (1)  ;

	return  0;						// should never get here!
}



/*
 *  get_fattime      stub routine, used by FatFS
 *
 *  Original version by Martin Thomas in his STM32 FatFS port.
 */
DWORD  get_fattime (void)
{
	DWORD res;
//	RTC_t rtc;

//	rtc_gettime( &rtc );
//	
	res =  (((DWORD)2014 - 1980) << 25)
			| ((DWORD)6 << 21)
			| ((DWORD)6 << 16)
			| (WORD)(18 << 11)
			| (WORD)(47 << 5)
			| (WORD)(30 >> 1);

	return res;
}


/*
 *  select      select (enable) the SD card
 */
static  void  select(void)
{
	CS_LOW;
}



/*
 *  deselect      deselect (disable) the SD card.
 */
static  void  deselect(void)
{
	CS_HIGH;
}



/*
 *  xchg      exchange a byte of data with the SD card via host's SPI bus
 */
static  char  xchg(char  c)
{
	return  SPIExchange(MY_SPI, c);
}



/*
 *  type_file      open a file and print the first few lines of text
 *
 *  This function reuses some of the global variables.
 */
static void  type_file(char  *fn)
{
	int					len;

	fres = f_open(&fil, readname, FA_READ);
	if (fres == FR_OK)
	{
		len = 0;
		while ((int)f_gets(buff, MAX_BUFF_LEN, &fil) != EOF)
		{
			xputs(buff);
			len++;
			if (len == 20)  break;				// don't fill the whole screen!
		}
		f_close(&fil);
	}
	else
	{
		xprintf("\n\rError:  Cannot open file %s", fn);
	}
}



/*
 *  scan_files      search given directory for files/subdirectories
 *
 *  Adapted from ChaN's example code for f_readdir().
 */
static FRESULT  scan_files (char  *path)
{
	int					i;
    FRESULT				res;
    FILINFO				fno;
    DIR					dir;
    char				*fn;   /* This function is assuming non-Unicode cfg. */

#if _USE_LFN
    static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif

/*
 *  First, display all directories in order.
 */
	res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK)
	{
        for (;;)
		{
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (fno.fattrib & AM_DIR)                    /* It is a directory */
			{
				xprintf("\n\r%s ", fn);
				i = 20-strlen(fn);
				while (i-- > 0)  xputc(' ');
				xprintf("---");
            }
        }
        f_closedir(&dir);
    }

/*
 *  Now, reopen the directory and display all the files in order.
 */
	res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK)
	{
        for (;;)
		{
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if ((fno.fattrib & AM_DIR) == 0)		// if this is a file...
			{
                xprintf("\n\r%s ", fn);
				i = 20-strlen(fn);
				while (i-- > 0)  xputc(' ');
				xprintf("%ld", fno.fsize);
            }
		}
		f_closedir(&dir);
	}
	return res;
}




