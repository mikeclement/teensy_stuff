/*
 *  printtest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will set up a UART for the console.  It will then
 *  invoke several of the termio routines, using that UART as the
 *  console device.
 */

#include  <stdio.h>
#include  <string.h>
#include  <stdint.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "uart.h"
#include  "termio.h"

#define  LED_ON		GPIOC_PSOR=(1<<5)
#define  LED_OFF	GPIOC_PCOR=(1<<5)

#define  MAX_BUFF_LEN		256

const char				hello[] = "\n\rprinttest\r\n";
char					buff[MAX_BUFF_LEN+1];
int32_t					ind;
int32_t					line;

int  main(void)
{
	PORTC_PCR5 = PORT_PCR_MUX(0x1); // LED is on PC5 (pin 13), config as GPIO (alt = 1)
	GPIOC_PDDR = (1<<5);			// make this an output pin
	LED_OFF;						// start with LED off

	UARTInit(TERM_UART, TERM_BAUD);	// open UART for comms
	xputs(hello);

	xputs("Type some stuff, then hit Enter....\n\r");

	line = 1;
	while (1)
	{
		ind = 0;
		while (get_line_r(buff, MAX_BUFF_LEN, &ind) == 0)
		{
			/*  I could add some background tasks here, if I wanted... */
		}
		xprintf("Line %d:0x%04x: %s\r\n", line, line, buff);
		line++;
	}

	return  0;						// should never get here!
}

