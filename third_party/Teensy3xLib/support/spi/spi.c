/*
 *  spi.c      library of SPI support code for Teensy 3.x
 */

#include  <stdio.h>
#include  <stdint.h>
#include  "common.h"
#include  "arm_cm4.h"

static  uint32_t			spi2_nbits;
static  uint32_t			spi2_mask;


#define  SPI2_SCK_LOW		GPIOD_PCOR=(1<<1)
#define  SPI2_SCK_HIGH		GPIOD_PSOR=(1<<1)

#define  SPI2_MOSI_LOW		GPIOC_PCOR=(1<<0)
#define  SPI2_MOSI_HIGH		GPIOC_PSOR=(1<<0)

#define  SPI2_MISO_INPUT	(GPIOB_PDIR&(1<<0))


/*
 *  SPIInit      initialize hardware SPI
 */
uint32_t  SPIInit(uint32_t  spinum, uint32_t  sckfreqkhz, uint32_t  numbits)
{
	SPI_MemMapPtr				spi;
	uint32_t					ctar;
	uint32_t					rate;
	uint32_t					brscaler;
	uint32_t					final;

	if ((numbits < 4) || (numbits > 16)) return  0;	// ignore illegal transfer sizes
	if (sckfreqkhz == 0)  return 0;				// ignore illegal clock freq

	if  (spinum == 0)
	{
		spi = SPI0_BASE_PTR;
		SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK;		// enable clocks to SPI0
	}
	else if (spinum == 1)
	{
		spi = SPI1_BASE_PTR;
		SIM_SCGC6 |= SIM_SCGC6_SPI1_MASK;		// enable clocks to SPI1
	}
	else if (spinum == 2)
	{
	}
	else  return  0;

	if (spinum < 2)
	{
		SPI_MCR_REG(spi) = SPI_MCR_MDIS_MASK | SPI_MCR_HALT_MASK;	// disable and halt SPI

		ctar = 0;									// use default CTAR0 (set for 8-bit data)
		rate = core_clk_khz / 2 / sckfreqkhz;		// assumes baud prescaler of /2
		final = core_clk_khz / 2;					// assumes final freq has /2 prescaler

		if (rate < 2)								// if requested rate is too high for regular rate...
		{
			rate = rate / 2;						// scale target freq for baud-rate doubling
			final = final / 2;						// keep the final frequency calc correct
			ctar = SPI_CTAR_DBR_MASK;				// set CTAR bit for baud-rate doubling
		}

		brscaler = 1;
		while ((1<<brscaler) < rate)
		{
			brscaler++;
		}
		if (brscaler > 15)  return  0;				// if out of range, give up; no SPI

		ctar |= brscaler;							// merge baud-rate doubler (if used) with scaler
		ctar |= ((numbits-1)<<SPI_CTAR_FMSZ_SHIFT);		// add in requested frame size
		SPI_CTAR_REG(spi, 0) = ctar;				// update attribute register CTAR0 for master mode
		SPI_MCR_REG(spi) = SPI_MCR_MSTR_MASK;		// enable SPI0 in master mode

		final = final / (1 << brscaler);			// calc the actual SPI frequency
	}
	else											// bit-banged SPI channel
	{
		spi2_nbits = numbits;
		spi2_mask = (1<<(numbits-1));
		final = core_clk_khz / 32;					// just a guess...
	}
/*
 *  Configure the I/O pins associated with SPI (SCK, MISO, MOSI).
 */
	if (spinum == 0)
	{
		PORTC_PCR5 = PORT_PCR_MUX(0x2);			// PC5 is SCK (alt2); Teensy3.1 pin 13 (LED)
		PORTC_PCR6 = PORT_PCR_MUX(0x2);			// PC6 is MOSI (alt2); Teensy3.1 pin 11
		PORTC_PCR7 = PORT_PCR_MUX(0x2);			// PC7 is MISO (alt2); Teensy3.1 pin 12
	}
	else if (spinum == 1)						//
	{
 		PORTB_PCR16 = PORT_PCR_MUX(0x2);		// PB16 is MOSI (alt2); Teensy3.1 pin 0
		PORTB_PCR17 = PORT_PCR_MUX(0x2);		// PB17 is MISO (alt2); Teensy3.1 pin 1
	}
	else if (spinum == 2)
	{
		GPIOD_PDDR |= (1<<1);					// PD1 is SCK, define as output
		PORTD_PCR1 = PORT_PCR_MUX(0x01);		// GPIO is alt1
		SPI2_SCK_LOW;							// SCK idles low (CPHA=0, CPOL=0)

		GPIOC_PDDR |= (1<<0);					// PC0 is MOSI, define as output
		PORTC_PCR0 = PORT_PCR_MUX(0x01);		// GPIO is alt1
		SPI2_MOSI_LOW;							// MOSI starts out low

		GPIOB_PDDR &= ~(1<<0);					// PB0 is MISO, define as input
		PORTB_PCR0 = PORT_PCR_MUX(0x01) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;		// GPIO is alt1
	}

	return  final;								// tell the world what we got
}




/*
 *  SPIExchange      exchange one byte with selected SPI channel
 */
uint32_t  SPIExchange(uint32_t  spinum, uint32_t  c)
{
	SPI_MemMapPtr				spi;
	register  uint32_t			mask;
	register  uint32_t			nbits;
	register  uint32_t			value;

	if (spinum == 2)				// for SPI2
	{
		nbits = spi2_nbits;			// need register version
		mask = spi2_mask;			// need register version
		value = 0;					// store incoming data in register
		while (nbits)				// for all bits in transfer...
		{
			if (c & mask)  SPI2_MOSI_HIGH;
			else           SPI2_MOSI_LOW;
			SPI2_SCK_HIGH;
			if (SPI2_MISO_INPUT)  value = value | mask;
			SPI2_SCK_LOW;
			mask = mask >> 1;
			nbits--;
		}
		return  value;				// done with SPI2, return early
	}

	if (spinum == 0)				// for SPI0
	{
		spi = SPI0_BASE_PTR;
	}
	else if (spinum == 1)			// for SPI1
	{
		spi = SPI1_BASE_PTR;
	}
	else  return  0;				// not legal SPI selector, just error out

/*
 *  We get here if selected SPI channel is either 0 or 1.  Variable
 *  spi holds the base register address for the channel.
 */
	SPI_SR_REG(spi) = SPI_SR_TCF_MASK;				// write 1 to the TCF flag to clear it
	SPI_PUSHR_REG(spi) = SPI_PUSHR_TXDATA((uint16_t)c);		// write data to Tx FIFO
	while ((SPI_SR_REG(spi) & SPI_SR_TCF_MASK) == 0)  ;	// lock until transmit complete flag goes high
	c = SPI_POPR_REG(spi);
	return  c;
}



/*
 *  SPISend      send one byte to selected SPI channel
 */
uint32_t  SPISend(uint32_t  spinum, uint32_t  c)
{
	SPI_MemMapPtr				spi;
	register  uint32_t			mask;
	register  uint32_t			nbits;

	if (spinum == 2)				// for SPI2
	{
		nbits = spi2_nbits;			// need register version
		mask = spi2_mask;			// need register version
		while (nbits)				// for all bits in transfer...
		{
			if (c & mask)  SPI2_MOSI_HIGH;
			else           SPI2_MOSI_LOW;
			SPI2_SCK_HIGH;
			SPI2_SCK_LOW;
			mask = mask >> 1;
			nbits--;
		}
		return  c;					// done with SPI2, return early
	}

	if (spinum == 0)				// for SPI0
	{
		spi = SPI0_BASE_PTR;
	}
	else if (spinum == 1)			// for SPI1
	{
		spi = SPI1_BASE_PTR;
	}
	else  return  0;				// not legal SPI selector, just error out

/*
 *  We get here if selected SPI channel is either 0 or 1.  Variable
 *  spi holds the base register address for the channel.
 */
	SPI_SR_REG(spi) = SPI_SR_TCF_MASK;				// write 1 to the TCF flag to clear it
	SPI_PUSHR_REG(spi) = SPI_PUSHR_TXDATA((uint16_t)c);		// write data to Tx FIFO
	while ((SPI_SR_REG(spi) & SPI_SR_TCF_MASK) == 0)  ;	// lock until transmit complete flag goes high
	return  c;
}

