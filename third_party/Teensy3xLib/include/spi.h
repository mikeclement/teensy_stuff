/*
 *  spi.h      header file for the Teensy 3.x SPI support library
 *
 *  This library supports the following SPI channels on the
 *  Teensy 3.1:
 *
 *  SPI0: supports all signals, master-mode only, no interrupt
 *        support, CPHA=0, CPOL=0
 *  SPI1: only supports MOSI and MISO (SCK not bounded out on the
 *        64-pin KL20 device used on Teensy 3.1), master-mode only,
 *        no interrupt support, CPHA=0, CPOL=0
 *  SPI2: bit-banged SPI at fixed clock; master-mode only, no
 *        interrupt support, CPHA=0, CPOL=0
 *
 *  Although SPI1 and SPI2 are not full-featured, they are still
 *  useful under the right circumstances.  SPI1 can generate
 *  clocked bit streams on MOSI, which can be used to drive
 *  certain devices that use a predefined clock.  SPI2 can
 *  act as a master for devices that require occasional I/O
 *  and are not restricted on SCLK frequency.
 *
 *  This library uses the following pin assignments for the
 *  supported SPI channels:
 *
 *	SPI0: SCK = PC5 (Teensy 3.1 pin labeled 13, also LED)
 *        MOSI = PC6 (Teensy 3.1 pin labeled 11)
 *        MISO = PC7 (Teensy 3.1 pin labeled 12)
 *
 *  SPI1: SCK = NONE (No SCK signal available on Teensy 3.1)
 *        MOSI = PB16 (Teensy 3.1 pin labeled 0)
 *        MISO = PB17 (Teensy 3.1 pin labeled 1)
 *
 *  SPI2: SCK = PD1 (Teensy 3.1 pin labeled 14)
 *        MOSI = PC0 (Teensy 3.1 pin labeled 15)
 *        MISO = PB0 (Teensy 3.1 pin labeled 16)
 */

#ifndef  SPI_H
#define  SPI_H



/*
 *  SPIInit      initialize selected SPI channel
 *
 *  This routine initializes the SPI channel selected by argument
 *  spinum.  The SPI baud rate is defined by the value passed in
 *  spifreqkhz.
 *
 *  The SPI channel is always configured as master.  SPI format is
 *  always CPHA=0, CPOL=0.
 *
 *  SPI baud rate is specified in kHz.  This routine will calculate the
 *  SPI settings based on the current core clock.
 *
 *  Argument spinum selects the SPI channel to use; legal values are
 *  0 through 2.  Argument sckfreqkhz selects the desired SPI clock
 *  frequency in kHz.  Argument numbits selects the number of bits
 *  in a transfer; legal values are 4 through 16.
 *
 *  SPI channel 2 is bit-banged, so the sckfreqkhz value will be ignored,
 *  but you cannot pass in 0; send 1 or whatever else you like.
 *
 *  Upon exit, this routine returns the final SPI clock frequency,
 *  which may be less than the requested frequency, but will not exceed
 *  it.  If this routine cannot set the requested frequency, it returns
 *  0 and the SPI is NOT initialized or enabled!
 *
 *  SPI channel 2 is bit-banged, so the value it returns is close to
 *  the final frequency but is not exact.  The final frequency will
 *  be close to 2 MHz.
 *
 *  NOTE: This routine does NOT perform any I/O of a chip-select line;
 *  that init must be done by external code.
 *
 *  NOTE: This routine does NOT use the SPI-based chip-selects; external
 *  code must assign chip-select to a GPIO pin and must handle all enable/
 *  disable functions using that pin.
 */
uint32_t				SPIInit(uint32_t  spinum,
								uint32_t  sckfreqkhz,
								uint32_t  numbits);


/*
 *  SPIExchange      exchange a data value over the selected SPI channel
 *
 *  This routine sends the value in argument c to the SPI channel
 *  selected in argument spinum (0-2).  Upon exit, this routine
 *  returns the value read from the SPI channel.
 */
uint32_t				SPIExchange(uint32_t  spinum, uint32_t  c);


/*
 *  SPISend      sends a data value over the selected SPI channel
 *
 *  This routine sends the value in argument c to the SPI channel
 *  selected in argument spinum (0-2).  Upon exit, this routine
 *  returns the original argument c.
 */
uint32_t				SPISend(uint32_t  spinum, uint32_t  c);


#endif
