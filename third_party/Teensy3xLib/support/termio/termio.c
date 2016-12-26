/*
 *  termio.c      clones of several essential terminal I/O routines
 *
 *  I created these because I could not get the standard newlib iprintf
 *  and printf functions to work.  If I ever figure out the problem, I will
 *  drop these and switch over to the newlib code.
 *
 *  These routines rely on lower-level code that is target-specific.  The
 *  target-specific routines use calls to UART functions for terminal
 *  I/O operations, such as UART read, UART write, and accessing the UART
 *  character queues.  You will need to supply these routines.  This code
 *  was written to use routines in my UART library (uart.h and uart.c).  You
 *  can either use those routines (available on my website) or code your
 *  own.
 *
 *  These routines were taken from Martin Thomas' website; he used them
 *  in an STM project.  See http://gandalf.arubi.uni-kl.de/avr_projects/arm_projects/index_cortex.html.
 *
 *  Karl Lunt, 26 Dec 2011
 */
 
// Basic code from ChaN, minor modifications by Martin Thomas

#include  <stdio.h>
#include  <string.h>
#include  <stdint.h>
#include  <stdarg.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "uart.h"
#include  "termio.h"



int32_t  xatoi (char **str, long *res)
{
	uint32_t			val;
	uint8_t				c;
	uint8_t				radix;
	uint8_t				s;
	
	s = 0;

	while ((c = **str) == ' ') (*str)++;
	if (c == '-')
	{
		s = 1;
		c = *(++(*str));
	}

	if (c == '0')
	{
		c = *(++(*str));
		if (c <= ' ')
		{
			*res = 0;
			return 1;
		}
		if (c == 'x')
		{
			radix = 16;
			c = *(++(*str));
		}
		else
		{
			if (c == 'b')
			{
				radix = 2;
				c = *(++(*str));
			}
			else
			{
				if ((c >= '0')&&(c <= '9'))
					radix = 8;
				else
					return 0;
			}
		}
	}
	else
	{
		if ((c < '1')||(c > '9'))	return 0;
		radix = 10;
	}
	val = 0;
	while (c > ' ')
	{
		if (c >= 'a') c -= 0x20;
		c -= '0';
		if (c >= 17)
		{
			c -= 7;
			if (c <= 9) return 0;
		}
		if (c >= radix) return 0;
		val = val * radix + c;
		c = *(++(*str));
	}
	if (s) val = -val;
	*res = val;
	return 1;
}


// -----------------------------------------------------------

/*
 *  The following set of functions are wrappers for the newlib stdio
 *  character I/O functions.
 *
 *  Programs should call these functions (and other library functions
 *  in this module), rather than invoking _read, _write, or _avail
 *  directly.
 */

/*
 *  xputc      write a single char to the active UART.
 */

void xputc (char c)
{
	if (c == '\n') UARTWrite("\r", 1);
	UARTWrite(&c, 1);
}



/*
 *  xgetc      read a single char from the active UART, locks until char available
 */
uint8_t  xgetc(void)
{
	char				c;

	UARTRead(&c, 1);
	return  c;
}



/*
 *  xavail      test for available char from stdin
 *
 *  This routine returns 0 if no char is currently available from
 *  the active UART, else it returns the number of available chars,
 *  which must be at least 1.
 */
int32_t  xavail(void)
{
	return  UARTAvail();
}


// ----------------------------------------------------------------

/*
 *  Additional functions available to external programs for exchanging
 *  chars with the active UART.
 */

void  xputs (const char* str)
{
	while (*str)
		xputc(*str++);
}




void  xitoa (long val, int32_t radix, int32_t len)
{
	uint8_t				c;
	uint32_t			r;
	uint8_t				sgn;
	uint8_t				pad;
	uint8_t				s[20];
	uint8_t				i;
	long				v;

	pad = ' ';
	sgn = 0;
	i = 0;

	if (radix < 0)
	{
		radix = -radix;
		if (val < 0)
		{
			val = -val;
			sgn = '-';
		}
	}
	v = val;
	r = radix;
	if (len < 0)
	{
		len = -len;
		pad = '0';
	}
	if (len > 20) return;
	do
	{
		c = (uint8_t)(v % r);
		if (c >= 10) c += 7;
		c += '0';
		s[i++] = c;
		v /= r;
	} while (v);

	if (sgn) s[i++] = sgn;
	while (i < len)
		s[i++] = pad;
	do
		xputc(s[--i]);
	while (i);
}




/*
 *  This is an integer-only version of printf, similar to iprintf.
 */
void  xprintf (const char* str, ...)
{
	va_list					arp;
	int						d;
	int						r;
	int						w;
	int						s;
	int						l;


	va_start(arp, str);

	while ((d = *str++) != 0)
	{
		if (d != '%')
		{
			xputc(d);
			continue;
		}
		if (*str == '%')			// if we need to escape the percent sign...
		{
			xputc(d);				// print the one we have in d
			str++;					// now step over the second one
			continue;				// and resume
		}

		d = *str++;
		w = 0;
		r = 0;
		s = 0;
		l = 0;

		if (d == '0')
		{
			d = *str++;
			s = 1;
		}
		while ((d >= '0')&&(d <= '9'))
		{
			w += w * 10 + (d - '0');
			d = *str++;
		}

		if (s)  w = -w;
		if (d == 'l')
		{
			l = 1;
			d = *str++;
		}
		if (!d)  break;
		if (d == 's')
		{
			xputs(va_arg(arp, char*));
			continue;
		}
		if (d == 'c')
		{
			xputc((char)va_arg(arp, int));
			continue;
		}
		if (d == 'u') r = 10;
		if (d == 'd') r = -10;
		if (d == 'X' || d == 'x') r = 16; // 'x' added by mthomas in increase compatibility
		if (d == 'b') r = 2;
		if (!r) break;
		if (l)
		{
			if (r > 0)
				xitoa((long)va_arg(arp, unsigned int), r, w);
			else
				xitoa((long)va_arg(arp, int), r, w);
		} else
		{
			if (r > 0)
				xitoa((long)va_arg(arp, unsigned int), r, w);
			else
				xitoa((long)va_arg(arp, int), r, w);
		}
	}

	va_end(arp);
}




void  put_dump (const uint8_t *buff, uint32_t ofs, int32_t cnt)
{
	uint8_t n;


	xprintf("%08lX ", ofs);
	for(n = 0; n < cnt; n++)
		xprintf(" %02X", buff[n]);
	xputc(' ');
	for(n = 0; n < cnt; n++) {
		if ((buff[n] < 0x20)||(buff[n] >= 0x7F))
			xputc('.');
		else
			xputc(buff[n]);
	}
	xputc('\n');
}




int32_t  get_line (char *buff, int32_t len)
{
	char						c;
	int32_t						idx;
	
	idx = 0;

	for (;;)
	{
		c = xgetc();
		if (c == '\r') break;
		if ((c == '\b') && idx)
		{
			idx--;
			xputc(c);
			xputc(' ');				// added by mthomas for Eclipse Terminal plug-in
			xputc(c);
		}
		if (((uint8_t)c >= ' ') && (idx < len - 1))
		{
			buff[idx++] = c; xputc(c);
		}
	}
	buff[idx] = 0;
	xputc('\n');
	return  idx;
}



/*
 *  get_line_r      accumulate chars into a buffer until \r
 *
 *  This routine will gather characters from stdin into a buffer
 *  (pointed to by argument buff).  At each invocation, this
 *  routine gathers all the chars available, up to a \r or a max
 *  of len chars, then returns a value that is 0 if the last char
 *  was not a \r or 1 if it was.
 *
 *  If this routine detects a \r from stdin, it discards that
 *  char and inserts a null into the string to mark the end; it
 *  also echoes a \n to stdout.  It then returns a value of 1 to
 *  show the string is complete.
 *
 *  If this routine detects a backspace ('\b') and there is
 *  at least one char already in the buffer, this routine will
 *  delete that char and decrement the buffer pointer.  This
 *  routine will also send a sequence of \b-space-\b to stdout,
 *  causing a terminal to remove the most recently typed char.
 *
 *  If this routine detects a printable char and there is room
 *  in the buffer, that char is echoed to stdout and stored in
 *  the buffer; the index pointed to by idx is incremented.
 *
 *  This routine is intended to be called repeatedly; it does
 *  not block.  This allows the calling routine to perform
 *  background tasks until a user enters a terminating \r.
 *  A typical example would be:
 *
 *    index = 0;
 *    while (get_line_r(buff, 80, &index) == 0)
 *    {
 *      <do some background stuff here>
 *    }
 *    <process input from user>
 *
 */ 
int32_t  get_line_r (char *buff, int32_t len, int32_t *idx)
{
	char					c;
	int						retval;
	int						myidx;

	retval = 0;						// assume no end yet
	myidx = *idx;					// get the working index into the buffer

	if (len < 1)  return  0;		// ignore edge case and outright errors

	if (xavail() && (myidx < len))	// if a char is available and there is room...
	{
		c = xgetc();				// get next char
		if (c == '\r')				// if end-of-line...
		{
			buff[myidx] = 0;		// end the string
			xputc('\n');			// echo a newline
			retval = 1;				// show we're done
		}
		else						// not end-of-line, how about backspace...
		{
			if ((c == '\b') && myidx)	// if backspace and at least 1 char in buffer...
			{
				myidx--;			// back up one char in buffer
				xputc(c);			// send the backspace
				xputc(' ');			// make it pretty
				xputc(c);			// now reposition cursor
			}
			if (((uint8_t)c >= ' ') && (myidx < len-1))	// if printable and still room for end-of-line...
			{
				buff[myidx++] = c;	// save char, bump index
				xputc(c);			// and echo the char
			}
		}
		*idx = myidx;				// all done, save updated pointer
	}

	return retval;
}


