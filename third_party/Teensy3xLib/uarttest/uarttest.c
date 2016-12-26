/*
 *  uarttest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will set up a UART for console.  It will then
 *  loop, waiting for chars from the serial port.  All chars will
 *  be echoed back to the UART.  Any CRs received will be followed
 *  by LFs.
 */

#include  <stdio.h>
#include  <string.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "uart.h"

#define  LED_ON		GPIOC_PSOR=(1<<5)
#define  LED_OFF	GPIOC_PCOR=(1<<5)

const char				hello[] = "uarttest\r\n";

int  main(void)
{
	char						c;

//	PORTC_PCR5 = PORT_PCR_MUX(0x1); // LED is on PC5 (pin 13), config as GPIO (alt = 1)
//	GPIOC_PDDR = (1<<5);			// make this an output pin
//	LED_OFF;						// start with LED off

	UARTInit(TERM_UART, TERM_BAUD);			// open UART for comms
	UARTWrite(hello, strlen(hello));
	EnableInterrupts;

	while (1)
	{
		while (UARTAvail() != 0)
		{
			UARTRead(&c, 1);
			UARTWrite(&c, 1);
			if (c == '\r')  UARTWrite("\n", 1);
		}
	}

	return  0;						// should never get here!
}


void  DefaultISR (void)
{
//	LED_ON;
	while(1)  ;
}

