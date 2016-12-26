/*
 *  uart.c      routines for low-level UART initialization
 *
 *  This file creates an object module (uart.o) that can be
 *  linked with other object modules, such as a datalogger
 *  project, to provide access to the Teensy 3.x UARTs.
 *
 *  This code is a mashup from code pulled from the web,
 *  including example code from the Freescale CodeWarrior
 *  library.  As far as I know, all parent code was in the
 *  public domain or was some variant of GPL.
 *  Karl Lunt, 11 May 2014
 */

#include  <stdio.h>
#include  <string.h>
#include  <stdint.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "uart.h"


#ifndef  FALSE
#define  FALSE  0
#define  TRUE  !FALSE
#endif

#define  MAX_RCV_Q_CHARS	64



volatile char				UART0RcvQ[MAX_RCV_Q_CHARS];
volatile uint8_t			UART0RcvOutIndex;
volatile uint8_t			UART0RcvInIndex;

volatile char				UART1RcvQ[MAX_RCV_Q_CHARS];
volatile uint8_t			UART1RcvOutIndex;
volatile uint8_t			UART1RcvInIndex;

volatile char				UART2RcvQ[MAX_RCV_Q_CHARS];
volatile uint8_t			UART2RcvOutIndex;
volatile uint8_t			UART2RcvInIndex;

static  UART_MemMapPtr		ActiveUARTBasePtr = 0;


/*
 *  Local functions
 */
static UART_MemMapPtr		XlateUARTNumToBasePtr(uint32_t  uartn);
static uint32_t				uart_char_avail(void);
static uint32_t				uart_putchar(char ch);
static char					uart_getchar(void);



void  UARTInit(uint32_t  uartnum, int32_t baud)
{
	UART_MemMapPtr					uartbase;
    register uint16_t				sbr;
	register uint16_t				brfa;
	uint32_t						sysclk;
    uint8_t							temp;

	uartbase = XlateUARTNumToBasePtr(uartnum);	// convert num (0-2) to UART base pointer
	if (uartbase == (UART_MemMapPtr) 0)  return;		// if no such UART, ignore

/*
 *  UART0 and UART1 are clocked from the core clock, but all other UARTs are
 *  clocked from the peripheral clock. So we have to determine which clock
 *  to use in baud rate calcs.
 */
    if ((uartbase == UART0_BASE_PTR) | (uartbase == UART1_BASE_PTR))
		sysclk = core_clk_khz;
    else
		sysclk = periph_clk_khz;

/*
 *  Enable the clock to the selected UART
 */
    if      (uartbase == UART0_BASE_PTR)  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
    else if (uartbase == UART1_BASE_PTR)  SIM_SCGC4 |= SIM_SCGC4_UART1_MASK;
    else                                  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;

    /* Make sure that the transmitter and receiver are disabled while we 
     * change settings.
     */
    UART_C2_REG(uartbase) &= ~(UART_C2_TE_MASK		// disable transmitter
							 | UART_C2_RE_MASK		// disable receiver
							 | UART_C2_RIE_MASK);	// disable receive interrupt on buffer full

    /* Configure the UART for 8-bit mode, no parity */
    UART_C1_REG(uartbase) = 0;	/* We need all default settings, so entire register is cleared */
    
    /* Calculate baud settings */
    sbr = (uint16_t)((sysclk*1000)/(baud * 16));
        
    /* Save off the current value of the UARTx_BDH except for the SBR field */
    temp = UART_BDH_REG(uartbase) & ~(UART_BDH_SBR(0x1F));
    
    UART_BDH_REG(uartbase) = temp |  UART_BDH_SBR(((sbr & 0x1F00) >> 8));
    UART_BDL_REG(uartbase) = (uint8_t)(sbr & UART_BDL_SBR_MASK);
    
    /* Determine if a fractional divider is needed to get closer to the baud rate */
    brfa = (((sysclk*32000)/(baud * 16)) - (sbr * 32));
    
    /* Save off the current value of the UARTx_C4 register except for the BRFA field */
    temp = UART_C4_REG(uartbase) & ~(UART_C4_BRFA(0x1F));
    
    UART_C4_REG(uartbase) = temp |  UART_C4_BRFA(brfa);    

    /* Enable receiver, transmitter and receiver interrupts */
	UART_C2_REG(uartbase) |= (UART_C2_TE_MASK
							| UART_C2_RE_MASK
							| UART_C2_RIE_MASK
							);

/*
 *  Make the connection to the external pins, based on the base pointer.
 */
	if (uartbase == UART0_BASE_PTR)
    {
        PORTB_PCR17 = PORT_PCR_MUX(0x3); // UART0 TXD is alt3 function on PB17
        PORTB_PCR16 = PORT_PCR_MUX(0x3); // UART0 RXD is alt3 function on PB16
    }
    if (uartbase == UART1_BASE_PTR)
  	{
       PORTC_PCR4 = PORT_PCR_MUX(0x3); // UART1 TXD is alt3 function on PC4
       PORTC_PCR3 = PORT_PCR_MUX(0x3); // UART1 RXD is alt3 function on PC3   
	}
  	if (uartbase == UART2_BASE_PTR)
  	{
  		PORTD_PCR3 = PORT_PCR_MUX(0x3); // UART2 TXD is alt3 function on PD3
  		PORTD_PCR2 = PORT_PCR_MUX(0x3); // UART2 RXD is alt3 function on PD2
  	}


/*
 *  Update NVIC to handle receiver interrupts
 *  (Refer to K20 Reference Manual and K20 Quick Reference Guide from Freescale)
 */
/*
 *  UART0 single interrupt vector for status sources
 *  Vector = 61
 *  IRQ = 45
 *  UART0 bit = IRQ mod 32 = 45 mod 32 = 13
 *  NVIC register offset (NVICSERx, etc) = IRQ / 32 = 45 / 32 = 1
 *  Priority is 0-15 (0 is highest), written to high four bits of NVICIP45.
 *
 */
	if (uartbase == UART0_BASE_PTR)
    {
		NVICICPR1 |= (1<<13);				// clear any pending interrupt
		NVICISER1 |= (1<<13);				// enable UART0 status source interrupt
		NVICIP45 = 0x30;					// set priority level for this IRQ to (pppp 0000)
	}
/*
 *  UART1 single interrupt vector for status sources
 *  Vector = 63
 *  IRQ = 47
 *  UART0 bit = IRQ mod 32 = 47 mod 32 = 15
 *  NVIC register offset (NVICSERx, etc) = IRQ / 32 = 45 / 32 = 1
 *  Priority is 0-15 (0 is highest), written to high four bits of NVICIP47.
 *
 */
    if (uartbase == UART1_BASE_PTR)
  	{
		NVICICPR1 |= (1<<15);				// clear any pending interrupt
		NVICISER1 |= (1<<15);				// enable UART1 status source interrupt
		NVICIP47 = 0x30;					// set priority level for this IRQ to (pppp 0000)
	}
/*
 *  UART2 single interrupt vector for status sources
 *  Vector = 65
 *  IRQ = 49
 *  UART0 bit = IRQ mod 32 = 45 mod 32 = 17
 *  NVIC register offset (NVICSERx, etc) = IRQ / 32 = 45 / 32 = 1
 *  Priority is 0-15 (0 is highest), written to high four bits of NVICIP49.
 *
 */
  	if (uartbase == UART2_BASE_PTR)
  	{
		NVICICPR1 |= (1<<17);				// clear any pending interrupt
		NVICISER1 |= (1<<17);				// enable UART2 status source interrupt
		NVICIP49 = 0x30;					// set priority level for this IRQ to (pppp 0000)
  	}

	ActiveUARTBasePtr = uartbase;			// done, record inited UART as active UART
}




uint32_t  UARTAssignActiveUART(uint32_t  uartnum)
{
	uint32_t				olduartnum;
	UART_MemMapPtr			tptr;

	if      (ActiveUARTBasePtr == UART0_BASE_PTR)  olduartnum = 0;
	else if (ActiveUARTBasePtr == UART1_BASE_PTR)  olduartnum = 1;
	else if (ActiveUARTBasePtr == UART2_BASE_PTR)  olduartnum = 2;
	else     olduartnum = (uint32_t) -1;

	tptr = XlateUARTNumToBasePtr(uartnum);
	if (tptr)  ActiveUARTBasePtr = tptr;
	return  olduartnum;
}




int32_t  UARTWrite(const char  *ptr, int32_t  len)
{
	int32_t					n;

	if (ActiveUARTBasePtr == 0)  return  0;

	n = 0;
	while (len)
	{
		n = n + uart_putchar(*ptr++ & (uint16_t)0xff);
		len--;
	}
	return  n;
}


int32_t  UARTAvail(void)
{
	if (ActiveUARTBasePtr == 0)  return  0;

	return  uart_char_avail();
}



int32_t  UARTRead(char *ptr, int32_t len)
{
	volatile char			c;
	int						chars;

	if (ActiveUARTBasePtr == 0)  return  0;			// don't try to read if no active UART

	for (chars=0; chars<len; chars++)
	{
		c = uart_getchar();							// go get a char
		*ptr = c;									// save the char we got
		ptr++;										// move to next cell
	}
	return  chars;
}




//            -------  static functions --------


/*
 *  XlateUARTNumToBasePtr      calc base pointer for selected UART
 *
 *  This routine translates a UART number (0-2) to a standard base
 *  pointer, for use with the Freescale register access macros.
 *
 *  If argument uartn is outside the legal range, this routine
 *  returns 0.
 */
static UART_MemMapPtr  XlateUARTNumToBasePtr(uint32_t  uartn)
{
	UART_MemMapPtr				r;

	if (uartn == 0)       r = UART0_BASE_PTR;
	else if (uartn == 1)  r = UART1_BASE_PTR;
	else if (uartn == 2)  r = UART2_BASE_PTR;
	else  r = (UART_MemMapPtr) 0;

	return  r;
}


/*
 *  uart_putchar      generic UART output routine
 *
 *  This routine locks until the requested UART has space available
 *  in its transmit FIFO, then writes the specified character to the
 *  transmit FIFO.
 *
 *  Upon entry, ch holds the character to send.
 *
 *  Returns 1 if able to write char to UART, else returns 0.
 */
static uint32_t  uart_putchar(char ch)
{
	if (ActiveUARTBasePtr == 0)  return 0;		// if no active UART, show no chars sent

    while (!(UART_S1_REG(ActiveUARTBasePtr) & UART_S1_TDRE_MASK))  ;	// lock until ready
    UART_D_REG(ActiveUARTBasePtr) = (uint8_t)ch;	// write char to UART
	return  1;					// return number of chars sent
 }



/*
 *  uart_char_avail      return state of receive FIFO
 *
 *  This routine checks the active UART for available chars and returns
 *  the available count.
 *
 *  If receive interrupts are enabled, the value returned is the number of
 *  chars in the receive queue.  If recieve interrupts are not enabled,
 *  the value returned is 1 if there is at least one char in the UART
 *  recieve FIFO.
 *
 *  If there is no active UART, this routine returns 0.
 *
 */
static uint32_t  uart_char_avail(void)
{
	int32_t					c;

	if (UART_C2_REG(ActiveUARTBasePtr) & UART_C2_RIE_MASK)	// if rcv interrupt is enabled...
	{
		switch  ((uint32_t)ActiveUARTBasePtr)
		{
			case  (uint32_t)UART0_BASE_PTR:
			c = (UART0RcvInIndex - UART0RcvOutIndex);	// race condition!  need to turn off interrupts?
			if (c < 0)  c = -c;
			break;

			case  (uint32_t)UART1_BASE_PTR:
			c = (UART1RcvInIndex - UART1RcvOutIndex);	// race condition!  need to turn off interrupts?
			if (c < 0)  c = -c;
			break;

			case  (uint32_t)UART2_BASE_PTR:
			c = (UART2RcvInIndex - UART2RcvOutIndex);	// race condition!  need to turn off interrupts?
			if (c < 0)  c = -c;
			break;

			default:								// not a known UART, skip it
			c = 0;
			break;
		}
	}
	else
	{
		if (UART_S1_REG(ActiveUARTBasePtr) & UART_S1_RDRF_MASK)  c = 1;
		else  c = 0;
	}

	return  (uint32_t) c;
}




/*
 *  uart_getchar      get one char (with blocking) from active UART
 *
 *  This routine waits until a char is available in the receive
 *  FIFO of the active UART, then returns the oldest char.
 *
 *  If there is no active UART, this routine returns NULL.
 *
 */
static char  uart_getchar(void)
{
	char				c;

	while (uart_char_avail() == 0)  ;				// lock until char is available

	if (UART_C2_REG(ActiveUARTBasePtr) & UART_C2_RIE_MASK)	// if rcv interrupt is enabled...
	{
		switch  ((uint32_t) ActiveUARTBasePtr)
		{
			case  (uint32_t)UART0_BASE_PTR:
			c = UART0RcvQ[UART0RcvOutIndex];		// get next available char
			UART0RcvOutIndex++;						// bump the index
			UART0RcvOutIndex %= MAX_RCV_Q_CHARS;	// keep index legal
			break;

			case  (uint32_t)UART1_BASE_PTR:
			c = UART1RcvQ[UART1RcvOutIndex];		// get next available char
			UART1RcvOutIndex++;						// bump the index
			UART1RcvOutIndex %= MAX_RCV_Q_CHARS;	// keep index legal
			break;

			case  (uint32_t)UART2_BASE_PTR:
			c = UART2RcvQ[UART2RcvOutIndex];		// get next available char
			UART2RcvOutIndex++;						// bump the index
			UART2RcvOutIndex %= MAX_RCV_Q_CHARS;	// keep index legal
			break;

			default:
			return  0;
		}
	}
	else
	{
		c = UART_D_REG(ActiveUARTBasePtr);		// get char from UART FIFO
	}
	return  c;
}



/*
 *    -------------------  UART interrupt handlers  -------------------------
 */

void  UART0_RX_TX_IRQHandler(void)
{
	char			d;

	d = UART_S1_REG(UART0_BASE_PTR);			// first part of clearing the interrupt
	if ((d & UART_S1_RDRF_MASK) == 0)			// if this is not a rcv interrupt...
		return;

	d = UART_D_REG(UART0_BASE_PTR);				// get the received char
	UART0RcvQ[UART0RcvInIndex] = d;				// save in queue
	UART0RcvInIndex++;							// move to next cell
	UART0RcvInIndex %= MAX_RCV_Q_CHARS;			// keep index in legal range
	if (UART0RcvInIndex == UART0RcvOutIndex)	// if we filled the buffer...
	{
		UART0RcvOutIndex++;						// wipe out oldest char in buffer (good as any other solution)
		UART0RcvOutIndex %= MAX_RCV_Q_CHARS;	// keep index in legal range
	}
}



void  UART1_RX_TX_IRQHandler(void)
{
	char			d;

	d = UART_S1_REG(UART1_BASE_PTR);			// first part of clearing the interrupt
	if ((d & UART_S1_RDRF_MASK) == 0)			// if this is not a rcv interrupt...
		return;

	d = UART_D_REG(UART1_BASE_PTR);				// get the received char
	UART1RcvQ[UART1RcvInIndex] = d;				// save in queue
	UART1RcvInIndex++;							// move to next cell
	UART1RcvInIndex %= MAX_RCV_Q_CHARS;			// keep index in legal range
	if (UART1RcvInIndex == UART1RcvOutIndex)	// if we filled the buffer...
	{
		UART1RcvOutIndex++;						// wipe out oldest char in buffer (good as any other solution)
		UART1RcvOutIndex %= MAX_RCV_Q_CHARS;	// keep index in legal range
	}
}



void  UART2_RX_TX_IRQHandler(void)
{
	char			d;

	d = UART_S1_REG(UART2_BASE_PTR);			// first part of clearing the interrupt
	if ((d & UART_S1_RDRF_MASK) == 0)			// if this is not a rcv interrupt...
		return;

	d = UART_D_REG(UART2_BASE_PTR);				// get the received char
	UART2RcvQ[UART2RcvInIndex] = d;				// save in queue
	UART2RcvInIndex++;							// move to next cell
	UART2RcvInIndex %= MAX_RCV_Q_CHARS;			// keep index in legal range
	if (UART2RcvInIndex == UART0RcvOutIndex)	// if we filled the buffer...
	{
		UART2RcvOutIndex++;						// wipe out oldest char in buffer (good as any other solution)
		UART2RcvOutIndex %= MAX_RCV_Q_CHARS;	// keep index in legal range
	}
}


