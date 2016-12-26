/*
 *  rdptest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will accept a string of math operations from the
 *  user via the console UART, parse the string using a call to
 *  rdp(), and display the result.
 *
 */

#include  <stdio.h>
#include  <string.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "uart.h"
#include  "rdp.h"
#include  "termio.h"

#define  LED_ON		GPIOC_PSOR=(1<<5)
#define  LED_OFF	GPIOC_PCOR=(1<<5)

const char				hello[] = "\n\rrdpttest\r\n";
void					MyHandler(char  chnl);

#define  MAX_STR_LEN  255
char					buff[MAX_STR_LEN+1];
int32_t					answer;
int32_t					error;


int  main(void)
{
//	PORTC_PCR5 = PORT_PCR_MUX(0x1); // LED is on PC5 (pin 13), config as GPIO (alt = 1)
//	GPIOC_PDDR = (1<<5);			// make this an output pin
//	LED_OFF;						// start with LED off

	UARTInit(TERM_UART, TERM_BAUD);			// open UART for comms
	xputs(hello);
	xputs("\n\rEnter a string to parse...\n\r");

	EnableInterrupts;

	while (1)
	{
		xputs("\n\rString: ");
		get_line(buff, MAX_STR_LEN);
		if (strlen(buff) > 0)
		{
			answer = 0;
			error = rdp(buff, &answer);
			if (error == RDP_OK)
			{
				xprintf("Answer = %d 0x%08x\n\r", answer, answer);
			}
			else
			{
				xprintf("ERROR: rdp() returned %d\n\r", error);
			}
		}
	}

	return  0;						// should never get here!
}
