/*
 *  termio.h      terminal I/O functions for exchanging chars with the active UART
 *
 *  These functions interface the Teensy 3.x UARTs with user programs, providing
 *  common char I/O functions such as string print and reading chars from a UART
 *  console.
 *
 *  These routines were taken from Martin Thomas' website; he used them
 *  in an STM project.
 *  See http://gandalf.arubi.uni-kl.de/avr_projects/arm_projects/index_cortex.html.
 *
 *  These routines are based on printf(), getc(), and others, but specifically
 *  coded to use the UART primitives in Karl Lunt's uart.c file.  Refer to the
 *  corresponding uart.h and uart.c files for details.
 *
 *  xprintf, xatoi, and xatoi are limited to integer values, with a maximum of
 *  32 bits, signed or unsigned.
 */

#ifndef  TERM_IO_H
#define  TERM_IO_H


int32_t			xatoi (char **str, long *res);
void			xitoa (long val, int32_t radix, int32_t len);
void			xputc (char c);
uint8_t			xgetc(void);
int32_t			xavail(void);
void			xputs (const char* str);
void			xprintf (const char* str, ...);
void			put_dump (const uint8_t *buff, uint32_t ofs, int32_t cnt);
int32_t			get_line (char *buff, int32_t len);
int32_t			get_line_r (char *buff, int32_t len, int32_t *idx);

#endif
