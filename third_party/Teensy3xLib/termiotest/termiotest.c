/*
 *  termiotest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will set up UART0 for 115 Kbaud, 8N1.  It will then
 *  initialize SPI0 for 400 kHz SPI comms to an SD card.  It will then
 *  perform a series of FatFS operations on the card.
 */

#include  <stdio.h>
#include  <string.h>
#include  <stdint.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "sdcard.h"
#include  "uart.h"
#include  "termio.h"

const char			hello[] = "\n\rtermiotest";


int  main(void)
{
	UARTInit(1, 115200);			// open UART1 for comms
	xprintf(hello);

	xprintf("\n\rTests of xprintf() function:");
	xprintf("\n\r0x7f using %%02x prints as %02x", 0x7f);
	xprintf("\n\r0x7fff using %%04x prints as %04x", 0x7fff);
	xprintf("\n\r0x7fffffff using %%08x prints as %08x", 0x7fffffff);
	xprintf("\n\r255 using %%u prints as %u", 255);
	xprintf("\n\r255 using %%d prints as %d", 255);
	xprintf("\n\r65535 using %%u prints as %u", 65535);
	xprintf("\n\r65535 using %%d prints as %d", 65535);
	xprintf("\n\r4294967295 using %%lu prints as %lu", 4294967295);
	xprintf("\n\r4294967295 using %%ld prints as %ld", 4294967295);

	while (1)  ;
}


