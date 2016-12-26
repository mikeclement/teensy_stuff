/*
 *  sdcardtest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will set up a UART as console.  It will then
 *  initialize SPI0 for 400 kHz SPI comms to an SD card.
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

#define  MAX_BUFF_LEN		256

const char				hello[] = "\n\rsdcardtest\r\n";
char					buff[MAX_BUFF_LEN+1];
uint32_t				sckfreqkhz;
uint8_t					ocr[4];
uint8_t					csd[16];
uint8_t					cid[16];
uint8_t					readdata[512];
uint8_t					writedata[512];
int32_t					status;


#define		CS_HIGH		GPIOD_PSOR=(1<<0)
#define		CS_LOW		GPIOD_PCOR=(1<<0)


/*
 *  Local functions
 */
static  void			select(void);
static  void			deselect(void);
static  char			xchg(char  c);

#define		MY_SPI		0

int  main(void)
{
	uint32_t						n;

	UARTInit(TERM_UART, TERM_BAUD);			// open UART for comms
	xprintf(hello);
	xprintf("\n\rcore_clk_khz = %ld", core_clk_khz);

	PORTD_PCR0 = PORT_PCR_MUX(0x1); // CS is on PD0 (pin 2), config as GPIO (alt = 1)
	GPIOD_PDDR = (1<<0);			// make this an output pin
	CS_HIGH;						// and deselect the SD card

	while (1)
	{
		sckfreqkhz = 400;				// use slow initial SPI clock
		xprintf("\n\rRequested freq = %ld, final freq = %ld", sckfreqkhz, SPIInit(MY_SPI, sckfreqkhz, 8));
		xprintf("\n\r");

		xprintf("\n\rInitializing SD card library.");
		SDRegisterSPI(select, xchg, deselect);
		SDInit();
		xprintf("\n\rSD card type is %d.", SDType);

		SDReadOCR(ocr);
		xprintf("\n\rOCR: %02x  %02x  %02x  %02x", ocr[0], ocr[1], ocr[2], ocr[3]);

		SDReadCSD(csd);
		xprintf("\n\rCSD: ");
		for (n=0; n<16; n++)  xprintf("%02x  ", csd[n]);

		SDReadCID(cid);
		xprintf("\n\rCID: ");
		for (n=0; n<16; n++)  xprintf("%02x  ", cid[n]);
		for (n=0; n<16; n++)
		{
			if ((cid[n] >= ' ') && (cid[n] <= 127))  xprintf("%c", cid[n]);
			else  xprintf(".");
		}

		sckfreqkhz = 1000;				// switch to fast SPI clock
		xprintf("\n\rRequested freq = %ld, final freq = %ld", sckfreqkhz, SPIInit(MY_SPI, sckfreqkhz, 8));
		xprintf("\n\r");

		xprintf("\n\rReading block 0 of card...");
		status = SDReadBlock(0, readdata);
		if (status != SDCARD_OK)
		{
			xprintf(" Error: status is %d", status);
		}
		else
		{
			xprintf("OK.");
			for (n=0; n<512; n++)
			{
				if ((n % 16) == 0)  xprintf("\n\r %3x: ", n);
				xprintf("%02x ", readdata[n]);
				writedata[n] = ~readdata[n];
			}

			xprintf("\n\rWriting inverse of block 0...", writedata[0]);
			status = SDWriteBlock(0, writedata);
			if (status != SDCARD_OK)
			{
				xprintf(" Error: status is %d", status);
			}
			else
			{
				xprintf("OK.");
				xprintf("\n\rRestoring original contents...");
				status = SDWriteBlock(0, readdata);
				if (status != SDCARD_OK)
				{
					xprintf(" Error: status is %d", status);
				}
				else
				{
					xprintf("OK.");
				}
			}
		}
		xputs("\n\r");
		xgetc();					// wait for user to type a key...
	}

	return  0;						// should never get here!
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






