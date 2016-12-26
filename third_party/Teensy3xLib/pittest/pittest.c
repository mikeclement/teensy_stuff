/*
 *  pittest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will set up all four PIT channels to output
 *  different chars to the UART; each channel will be set
 *  to appear at different intervals.  When each PIT timmer
 *  fires, the Teensy will write the corresponding ASCII
 *  char to the serial port.
 *
 *  You can press the chars 0 through 3 to toggle that channel's
 *  PIT timer on or off.
 */

#include  <stdio.h>
#include  <string.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "uart.h"
#include  "pit.h"
#include  "termio.h"

#define  LED_ON		GPIOC_PSOR=(1<<5)
#define  LED_OFF	GPIOC_PCOR=(1<<5)

const char				hello[] = "\n\rpittest\r\n";
void					MyHandler(char  chnl);

int  main(void)
{
	char			c;
	char			mask;

//	PORTC_PCR5 = PORT_PCR_MUX(0x1); // LED is on PC5 (pin 13), config as GPIO (alt = 1)
//	GPIOC_PDDR = (1<<5);			// make this an output pin
//	LED_OFF;						// start with LED off

	UARTInit(TERM_UART, TERM_BAUD);			// open UART for comms
	xputs(hello);
	xputs("\n\rPress 0 through 3 to toggle that channel's PIT interrupt...\n\r");

	PITInit(0, &MyHandler, 1000000, 0, 0);		// a 1-sec delay
	PITInit(1, &MyHandler, 0, periph_clk_khz * 1000, 0);	// another way of getting a 1-sec delay 
	PITInit(2, &MyHandler, 2500000, 0, 0);
	PITInit(3, &MyHandler, 3000000, 0, 0);

	mask = 0x0f;					// show all channels active
	EnableInterrupts;

	while (1)
	{
		c = xgetc();				// get char from user
		c = c - '0';				// assume 0-3
		if ((c >= 0) && (c <= 3))	// if valid char...
		{
			if (mask & (1<<c))		// if this timer is now enabled...
			{
				PITStop(c);			// disable it
				mask &= ~(1<<c);	// record in mask
			}
			else					// no, this timer is now disabled...
			{
				PITStart(c);		// reenable this timer
				mask |= (1<<c);		// record in mask
			}
		}
	}

	return  0;						// should never get here!
}


/*
 *  MyHandler      ISR for handling all PIT interrupts
 *
 *  Control enters this routine when any of the PIT interrupts fires.
 *  The argument chnl contains the indentifier (0-3) of the PIT
 *  channel causing the interrupt.
 *
 *  Note that doing console I/O inside an interrupt is usually
 *  a very bad idea; lower-priority interrupts are starved until
 *  control leaves the ISR.  Such starvation is not an issue for
 *  this simple test case, but it's still a bad idea.
 */
void  MyHandler(char  chnl)
{
	xputc('0' + chnl);
}

