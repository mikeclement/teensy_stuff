/*
 *  sdcard.c      SD/SDHC card support library
 */

#include  <stdint.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  "sdcard.h"
#include  "termio.h"		// only used for debug!


#ifndef  FALSE
#define  FALSE		0
#define  TRUE		!FALSE
#endif


/*
 *  Externally accessible variables
 */
uint32_t						SDType = SDTYPE_UNKNOWN;


/*
 *  Local variables
 */
uint8_t							registered = FALSE;
void							(*select)(void);
char							(*xchg)(char  val);
void							(*deselect)(void);
uint8_t							crctable[256];


/*
 *  Local functions
 */
static  int8_t  				sd_send_command(uint8_t  command, uint32_t  arg);
static  int8_t					sd_wait_for_data(void);
static void						sd_clock_and_release(void);

static void 					GenerateCRCTable(void);
static uint8_t 					AddByteToCRC(uint8_t  crc, uint8_t  b);




static void GenerateCRCTable(void)
{
    int i, j;
 
    // generate a table value for all 256 possible byte values
    for (i = 0; i < 256; i++)
    {
        crctable[i] = (i & 0x80) ? i ^ CRC7_POLY : i;
        for (j = 1; j < 8; j++)
        {
            crctable[i] <<= 1;
            if (crctable[i] & 0x80)
                crctable[i] ^= CRC7_POLY;
        }
    }
}


static uint8_t  AddByteToCRC(uint8_t  crc, uint8_t  b)
{
	return crctable[(crc << 1) ^ b];
}






	

int32_t  SDInit(void)
{
	int					i;
	int8_t				response;
//	volatile uint32_t	dly;


	if (registered == FALSE)  return  SDCARD_NOT_REG;

	SDType = SDTYPE_UNKNOWN;			// assume this fails
/*
 *  Begin initialization by sending CMD0 and waiting until SD card
 *  responds with In Idle Mode (0x01).  If the response is not 0x01
 *  within a reasonable amount of time, there is no SD card on the bus.
 */
	deselect();							// always make sure
	for (i=0; i<10; i++)				// send several clocks while card power stabilizes
		xchg(0xff);

	for (i=0; i<0x10; i++)
	{
		response = sd_send_command(SD_GO_IDLE, 0);	// send CMD0 - go to idle state
		if (response == 1)  break;
	}
	if (response != 1)
	{
		return  SDCARD_NO_DETECT;
	}

	sd_send_command(SD_SET_BLK_LEN, 512);		// always set block length (CMD6) to 512 bytes

	response = sd_send_command(SD_SEND_IF_COND, 0x1aa);	// probe to see if card is SDv2 (SDHC)
	if (response == 0x01)						// if card is SDHC...
	{
		for (i=0; i<4; i++)						// burn the 4-byte response (OCR)
		{
			xchg(0xff);
		}
		for (i=20000; i>0; i--)
		{
			response = sd_send_command(SD_ADV_INIT, 1UL<<30);
			if (response == 0)  break;
		}
		SDType = SDTYPE_SDHC;
	}
	else
	{
		response = sd_send_command(SD_READ_OCR, 0);
		if (response == 0x01)
		{
			for (i=0; i<4; i++)					// OCR is 4 bytes
			{
				xchg(0xff);					// burn the 4-byte response (OCR)
			}
			for (i=20000; i>0; i--)
			{
				response = sd_send_command(SD_INIT, 0);
				if (response == 0)  break;
//				for (dly=0; dly<1000; dly++)  ;		// spin-loop delay
			}
			sd_send_command(SD_SET_BLK_LEN, 512);
			SDType = SDTYPE_SD;
		}
	}

	sd_clock_and_release();					// always deselect and send final 8 clocks

/*
 *  At this point, the SD card has completed initialization.  The calling routine
 *  can now increase the SPI clock rate for the SD card to the maximum allowed by
 *  the SD card (typically, 20 MHz).
 */
	return  SDCARD_OK;					// if no power routine or turning off the card, call it good
}




int32_t  SDStatus(void)
{
	if (registered == FALSE)	   return  SDCARD_NOT_REG;
	if (SDType == SDTYPE_UNKNOWN)  return  SDCARD_NO_DETECT;
	if ((SDType == SDTYPE_SD) || (SDType == SDTYPE_SDHC))  return  SDCARD_OK;

	return  SDCARD_NO_DETECT;
}



#if 0
static  void  ShowBlock(void)
{
	uint32_t				i;
	uint8_t					str[17];

	str[16] = 0;
	str[0] = 0;			// only need for first newline, overwritten as chars are processed

	printf_P(PSTR("\n\rContents of block buffer:"));
	for (i=0; i<512; i++)
	{
		if ((i % 16) == 0)
		{
			printf_P(PSTR(" %s\n\r%04X: "), str, i);
		}
		printf_P(PSTR("%02X "), (uint8_t)block[i]);
		if (isalpha(block[i]) || isdigit(block[i]))  str[i%16] = block[i];
		else									     str[i%16] = '.';
	}
	printf_P(PSTR(" %s\n\r"), str);
}


	
static int8_t  ExamineSD(void)
{
	int8_t			response;

	response = ReadOCR();		// this fails with Samsung; don't test response until know why
	response = ReadCSD();
	if (response == SDCARD_OK)
	{
//		printf_P(PSTR(" ReadCSD is OK "));
		response = ReadCID();
	}
	if (response == SDCARD_OK)
	{
//		printf_P(PSTR(" ReadCID is OK "));
		response = ReadCardStatus();
	}

	return  response;
}
#endif



int32_t  SDReadOCR(uint8_t  *buff)
{
	uint8_t				i;
	int8_t				response;

	for (i=0; i<4;  i++)  buff[i] = 0;

	if (SDType == SDTYPE_SDHC)
	{
		response = sd_send_command(SD_SEND_IF_COND, 0x1aa);
		if (response != 0)
		{
			sd_clock_and_release();			// cleanup  
			return  SDCARD_RWFAIL;
		}
		for (i=0; i<4; i++)
		{
			buff[i] = xchg(0xff);
		}
		xchg(0xff);							// burn the CRC
	}
	else
	{
		response = sd_send_command(SD_READ_OCR, 0);
		if (response != 0x00)
		{
			sd_clock_and_release();			// cleanup  
			return  SDCARD_RWFAIL;
		}
		for (i=0; i<4; i++)					// OCR is 4 bytes
		{
			buff[i] = xchg(0xff);
		}
		xchg(0xff);
	}
    sd_clock_and_release();				// cleanup  
	return  SDCARD_OK;
}



int32_t  SDReadCSD(uint8_t  *buff)
{
	uint8_t			i;
	int8_t			response;

	for (i=0; i<16; i++)  buff[i] = 0;

	response = sd_send_command(SD_SEND_CSD, 0);
	response = sd_wait_for_data();
	if (response != (int8_t)0xfe)
	{
		return  SDCARD_RWFAIL;
	}

	for (i=0; i<16; i++)
	{
		buff[i] = xchg(0xff);
	}
	xchg(0xff);							// burn the CRC
    sd_clock_and_release();				// cleanup  
	return  SDCARD_OK;
}



int32_t  SDReadCID(uint8_t  *buff)
{
	uint8_t			i;
	int8_t			response;

	for (i=0; i<16; i++)  buff[i] = 0;

	response = sd_send_command(SD_SEND_CID, 0);
	response = sd_wait_for_data();
	if (response != (int8_t)0xfe)
	{
		return  SDCARD_RWFAIL;
	}

	for (i=0; i<16; i++)
	{
		buff[i] = xchg(0xff);
	}
	xchg(0xff);							// burn the CRC
    sd_clock_and_release();				// cleanup  
	return  SDCARD_OK;
}



int32_t  SDWriteCSD(uint8_t  *buff)
{
	int8_t				response;
	uint8_t				tcrc;
	uint16_t			i;

	response = sd_send_command(SD_PROGRAM_CSD, 0);
	if (response != 0)
	{
		return  SDCARD_RWFAIL;
	}
	xchg(0xfe);							// send data token marking start of data block

	tcrc = 0;
	for (i=0; i<15; i++)				// for all 15 data bytes in CSD...
	{
    	xchg(*buff);					// send each byte via SPI
		tcrc = AddByteToCRC(tcrc, *buff);		// add byte to CRC
	}
	xchg((tcrc<<1) + 1);				// format the CRC7 value and send it

	xchg(0xff);							// ignore dummy checksum
	xchg(0xff);							// ignore dummy checksum

	i = 0xffff;							// max timeout
	while (!xchg(0xFF) && (--i))  ;		// wait until we are not busy

    sd_clock_and_release();				// cleanup  
	if (i)  return  SDCARD_OK;			// return success
	else  return  SDCARD_RWFAIL;		// nope, didn't work
}





#if 0
static int8_t  ReadCardStatus(void)
{
	cardstatus[0] = sd_send_command(SD_SEND_STATUS, 0);
	cardstatus[1] = xchg(0xff);
//	printf_P(PSTR("\r\nReadCardStatus = %02x %02x"), cardstatus[0], cardstatus[1]);
	xchg(0xff);
	return  SDCARD_OK;
}
#endif




int32_t  SDReadBlock(uint32_t  blocknum, uint8_t  *buff)
{
    uint16_t					i;
	uint8_t						status;
	uint32_t					addr;

	if (!registered)  return  SDCARD_NOT_REG;		// if no SPI functions, leave now
	if (SDType == SDTYPE_UNKNOWN)  return  SDCARD_UNKNOWN;	// card type not yet known

/*
 *  Compute byte address of start of desired sector.
 *
 *  For SD cards, the argument to CMD17 must be a byte address.
 *  For SDHC cards, the argument to CMD17 must be a block (512 bytes) number.
 */
	if (SDType == SDTYPE_SD) 	addr = blocknum << 9;	// SD card; convert block number to byte addr
	else						addr = blocknum;	// SDHC card; use the requested block number

    sd_send_command(SD_READ_BLK, addr); // send read command and logical sector address
	status = sd_wait_for_data();		// wait for valid data token from card
	if (status != 0xfe)					// card must return 0xfe for CMD17
    {
		xprintf("\n\rSDReadBlock: sd_wait_for_data returned %d", status);
        sd_clock_and_release();			// cleanup 
        return  SDCARD_RWFAIL;			// return error code
    }

    for (i=0; i<512; i++)           	// read sector data
        buff[i] = xchg(0xff);

    xchg(0xff);                		 	// ignore CRC
    xchg(0xff);                		 	// ignore CRC

    sd_clock_and_release();				// cleanup  
    return  SDCARD_OK;					// return success       
}


#if 0
static int8_t  ModifyPWD(uint8_t  mask)
{
	int8_t						r;
	uint16_t					i;

	mask = mask & 0x07;					// top five bits MUST be 0, do not allow forced-erase!
	r = sd_send_command(SD_LOCK_UNLOCK, 0);
	if (r != 0)
	{
		return  SDCARD_RWFAIL;
	}
	xchg(0xfe);							// send data token marking start of data block

	xchg(mask);							// always start with required command
	xchg(pwd_len);						// then send the password length
	for (i=0; i<512; i++)				// need to send one full block for CMD42
	{
		if (i < pwd_len)
		{
    		xchg(pwd[i]);					// send each byte via SPI
		}
		else
		{
			xchg(0xff);
		}
	}

	xchg(0xff);							// ignore dummy checksum
	xchg(0xff);							// ignore dummy checksum

	i = 0xffff;							// max timeout
	while (!xchg(0xFF) && (--i))  ;		// wait until we are not busy

	if (i)  return  SDCARD_OK;			// return success
	else  return  SDCARD_RWFAIL;		// nope, didn't work
}
#endif


#if 0
static void  ShowErrorCode(int8_t  status)
{
	if ((status & 0xe0) == 0)			// if status byte has an error value...
	{
		printf_P(PSTR("\n\rDate error:"));
		if (status & ERRTKN_CARD_LOCKED)
		{
			printf_P(PSTR(" Card is locked!"));
		}
		if (status & ERRTKN_OUT_OF_RANGE)
		{
			printf_P(PSTR(" Address is out of range!"));
		}
		if (status & ERRTKN_CARD_ECC)
		{
			printf_P(PSTR(" Card ECC failed!"));
		}
		if (status & ERRTKN_CARD_CC)
		{
			printf_P(PSTR(" Card CC failed!"));
		}
	}
}
#endif



#if 0
static void  ShowCardStatus(void)
{
	ReadCardStatus();
	printf_P(PSTR("\r\nPassword status: "));
	if ((cardstatus[1] & 0x01) ==  0)  printf_P(PSTR("un"));
	printf_P(PSTR("locked"));
}
#endif



#if 0
static void  LoadGlobalPWD(void)
{
	uint8_t				i;

	for (i=0; i<GLOBAL_PWD_LEN; i++)
	{
		pwd[i] = pgm_read_byte(&(GlobalPWDStr[i]));
	}
	pwd_len = GLOBAL_PWD_LEN;
}
#endif




int32_t	 SDRegisterSPI(void	    (*pselect)(void),
					   char		(*pxchg)(char  val),
					   void     (*pdeselect)(void))
{
	uint32_t					result;

	GenerateCRCTable();				// need to do at some point, here is as good as any

	select = pselect;
	xchg = pxchg;
	deselect = pdeselect;

	result = SDCARD_OK;				// assume all pointers are at least believable
	registered = FALSE;
	if (select && xchg && deselect)  registered = TRUE;
	if (!registered)  result = SDCARD_REGFAIL;	// missing function handle
	return  result;
}



/*
 *  SDWriteBlock      write buffer of data to SD card
 *
 *  I've lost track of who created the original code for this
 *  routine; I found it somewhere on the web.
 *
 *	Write a single 512 byte sector to the MMC/SD card
 *  blocknum holds the number of the 512-byte block to write,
 *  buffer points to the 512-byte buffer of data to write.
 *
 *  Note that for SD (not SDHC) cards, blocknum will be converted
 *  into a byte address.
 */
int32_t  SDWriteBlock(uint32_t  blocknum, uint8_t  *buff)
{
	uint16_t				i;
	uint8_t					status;
	uint32_t				addr;

	if (!registered)  return  SDCARD_NOT_REG;

/*
 *  Compute byte address of start of desired sector.
 *
 *  For SD cards, the argument to CMD17 must be a byte address.
 *  For SDHC cards, the argument to CMD17 must be a block (512 bytes) number.
 */
	if (SDType == SDTYPE_SD)  addr = blocknum << 9;	// SD card; convert block number to byte addr
	else					  addr = blocknum;		// SDHC card; just use blocknum as is

	status = sd_send_command(SD_WRITE_BLK, addr);

	if (status != SDCARD_OK)			// if card does not send back 0...
	{
		sd_clock_and_release();			// cleanup
		return  SDCARD_RWFAIL;
	}

	xchg(0xfe);							// send data token marking start of data block

	for (i=0; i<512; i++)				// for all bytes in a sector...
	{
    	xchg(*buff++);					// send each byte via SPI
	}

	xchg(0xff);							// ignore dummy checksum
	xchg(0xff);							// ignore dummy checksum

	if ((xchg(0xFF) & 0x0F) != 0x05)
	{
		sd_clock_and_release();			// cleanup
		return  SDCARD_RWFAIL;
	}
	
	i = 0xffff;							// max timeout  (nasty timing-critical loop!)
	while (!xchg(0xFF) && (--i))  ;		// wait until we are not busy
	sd_clock_and_release();				// cleanup
	return  SDCARD_OK;					// return success		
}




/*
 *  sd_waitforready      wait until write operation completes
 *
 *  Normally not used, this function is provided to support Chan's FAT32 library.
 */
uint8_t  sd_waitforready(void)
{
   uint8_t				i;
   uint16_t				j;

	if (!registered)  return  SDCARD_NOT_REG;

    select();							// enable CS
    xchg(0xff);         				// dummy byte

	j = 5000;
	do  {
		i = xchg(0xFF);
	} while ((i != 0xFF) && --j);

	sd_clock_and_release();

	if (i == 0xff)  return  SDCARD_OK;	// if SD card shows ready, report OK
	else			return  SDCARD_RWFAIL;		// else report an error
}



/*
 *  ==========================================================================
 *
 *  sd_send_command      send raw command to SD card, return response
 *
 *  This routine accepts a single SD command and a 4-byte argument.  It sends
 *  the command plus argument, adding the appropriate CRC.  It then returns
 *  the one-byte response from the SD card.
 *
 *  For advanced commands (those with a command byte having bit 7 set), this
 *  routine automatically sends the required preface command (CMD55) before
 *  sending the requested command.
 *
 *  Upon exit, this routine returns the response byte from the SD card.
 *  Possible responses are:
 *    0xff	No response from card; card might actually be missing
 *    0x01  SD card returned 0x01, which is OK for most commands
 *    0x?? 	other responses are command-specific
 */
static  int8_t  sd_send_command(uint8_t  command, uint32_t  arg)
{
	uint8_t				response;
	uint8_t				i;
	uint8_t				crc;

	if (command & 0x80)					// special case, ACMD(n) is sent as CMD55 and CMDn
	{
		command = command & 0x7f;		// strip high bit for later
		response = sd_send_command(CMD55, 0);	// send first part (recursion)
		if (response > 1)  return response;
	}

	sd_clock_and_release();
	select();							// enable CS
	xchg(0xff);

    xchg(command | 0x40);				// command always has bit 6 set!
	xchg((unsigned char)(arg>>24));		// send data, starting with top byte
	xchg((unsigned char)(arg>>16));
	xchg((unsigned char)(arg>>8));
	xchg((unsigned char)(arg&0xff));
	crc = 0x01;							// good for most cases
	if (command == SD_GO_IDLE)  crc = 0x95;			// this will be good enough for most commands
	if (command == SD_SEND_IF_COND)  crc = 0x87;	// special case, have to use different CRC
    xchg(crc);         					// send final byte                          

	for (i=0; i<10; i++)				// loop until timeout or response
	{
		response = xchg(0xff);
		if ((response & 0x80) == 0)  break;	// high bit cleared means we got a response
	}

/*
 *  We have issued the command but the SD card is still selected.  We
 *  only deselect the card if the command we just sent is NOT a command
 *  that requires additional data exchange, such as reading or writing
 *  a block.
 */
	if ((command != SD_READ_BLK) &&
		(command != SD_WRITE_BLK) &&
		(command != SD_READ_OCR) &&
		(command != SD_SEND_CSD) &&
		(command != SD_SEND_STATUS) &&
		(command != SD_SEND_CID) &&
		(command != SD_SEND_IF_COND) &&
		(command != SD_LOCK_UNLOCK) &&
		(command != SD_PROGRAM_CSD))
	{
		sd_clock_and_release();
	}

	return  response;					// let the caller sort it out
}



static int8_t  sd_wait_for_data(void)
{
	int16_t				i;
	uint8_t				r;
	volatile uint32_t	dly;

	for (i=0; i<1000; i++)
	{
		r = xchg(0xff);
		if (r != 0xff)  break;
		for (dly=0; dly<1000; dly++)  ;		// spin-loop delay
	}
	return  (int8_t) r;
}



/*
 *  sd_clock_and_release      deselect the SD card and provide additional clocks
 *
 *  The SD card does not release its DO (MISO) line until it sees a clock on SCK
 *  AFTER the CS has gone inactive (Chan, mmc_e.html).  This is not an issue if the
 *  only device on the SPI bus is the SD card, but it can be an issue if the
 *  card shares the bus with other SPI devices.
 *
 *  To be safe, this routine deselects the SD card, then issues eight clock pulses.
 */
static void  sd_clock_and_release(void)
{
	if (!registered)  return;

	deselect();				   				// release CS
	xchg(0xff);								// send 8 final clocks
}










