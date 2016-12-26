/*
 *  spitest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will set up SPI1 and SPI2 (bit-banged SPI),
 *  then begin sending a fixed value to those channels.  Use
 *  an oscilloscope to monitor the waveforms.
 */

#include  <stdio.h>
#include  <string.h>
#include  <stdint.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "spi.h"
#include  "uart.h"
#include  "termio.h"

const char			hello[] = "\n\rspitest\n\r";


int  main(void)
{
	uint32_t			freq;
	uint32_t			v1;
	uint32_t			v2;

	UARTInit(TERM_UART, TERM_BAUD);			// open UART for comms
	xprintf(hello);

	freq = SPIInit(1, 10, 16);
	xprintf("\n\rSPIInit for SPI1 returns %d", freq);

	freq = SPIInit(2, 99, 16);
	xprintf("\n\rSPIInit for SPI2 returns %d", freq);

	xputs("\n\rSPI1: SCK (N/A)     MOSI (pin 0)   MISO (pin 1)");
	xputs("\n\rSPI2: SCK (pin 14)  MOSI (pin 15)  MISO (pin 16)");
	xputs("\n\rSending 0x5555 to both SPI channels...");
	xputs("\n\r\n\r");
		
		
	while (1)
	{
//		SPISend(1, 0x5555);
//		SPISend(2, 0x5555);

		v1 = SPIExchange(1, 0x5555);
		v2 = SPIExchange(2, 0x5555);

		xprintf("\rSPI1: %04x  SPI2: %04x   ", v1, v2);
	}
}


