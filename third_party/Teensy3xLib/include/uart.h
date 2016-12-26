/*
 * File:		uart.h
 * Purpose:     Provide common ColdFire UART routines for polled serial IO
 *
 * Notes:
 * Modified to hide getchar, putchar, and char_avail routines; those now exist
 * as statics inside the uart.c code.  Modified uart_init() to take a UART
 * number (0-2), rather than a UART base pointer, and to remove the clock
 * argument; baud calcs are now based on the selected UART.
 */

#ifndef __UART_H__
#define __UART_H__


/*
 *  These routines support access to all UARTs on the Teensy 3.x (K20).
 *  To use a UART, first call UARTInit() with the UART number (0-2) and
 *  a desired baud rate; this call marks the selected UART as active.
 *  You can then call UARTWrite() to send chars to the active UART.  Use
 *  UARTAvail() to check for received chars.  Use UARTRead() to read
 *  chars (with blocking) from the active UART.
 *
 *  To use a different UART as the active UART, call UARTAssignActiveUART().
 */


/*
 *  UARTInit      setup selected UART, including baud rate
 *
 *  Uses system core or peripheral clock (depending on UART) to calc
 *  divisors for selected baud rate.  Configures UART for polled I/O.
 *  Sets up GPIO lines as needed.
 *
 *  Upon entry, uartnum selects the UART (0-2) and baud is the
 *  desired baud rate, expressed as a uint32_t.  For example,
 *  a value of 115200 for baud generates a baud clock of 115 Kbaud.
 *
 *  Calls with an illegal UART selector for uartnum are ignored.
 */
void			UARTInit(uint32_t  uartnum, int32_t  baudrate);



/*
 *  UARTAssignActiveUART      assign an active UART for later char read/write
 *
 *  This routine assigns an active UART based on the argument uartnum.  This
 *  argument must be in the legal range of UARTs for the device; for Teensy 3.x,
 *  this range is 0-2.
 *
 *  Upon exit, this routine returns the previous active UART number, if there
 *  was such, else it returns uin32_t -1.
 *
 *  If the requested UART number is out of the legal range, this routine does
 *  not change the active UART.  However, it still returns the previous active
 *  UART number.
 */
uint32_t		UARTAssignActiveUART(uint32_t  uartnum);



/*
 *  UARTWrite      writes N chars to active UART
 *
 *  Upon entry, ptr points to a block (not string!) of
 *  chars to write to the active UART and len holds the
 *  number of chars to write.
 *
 *  Returns number of chars written.
 */

int32_t			UARTWrite(const char *ptr, int32_t len);




/*
 *  UARTAvail      returns number of chars waiting in active UART's holding area
 *
 */
int32_t			UARTAvail(void);




/*
 *  UARTRead      reads (with blocking) len chars from active UART, writes to buffer at ptr
 */
int32_t			UARTRead(char *ptr, int32_t len);


#endif /* __UART_H__ */
