/*
 *  pit.c      library of Periodic Interrupt Timer (PIT) support code for Teensy 3.x
 */

#include  <stdio.h>
#include  <stdint.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "pit.h"


void			(*pisr[4])(char);

/*
 *  PITInit      initialize PIT
 */
uint32_t  PITInit(uint32_t  chnl,
				  void (*isrhandler)(char),
				  uint32_t  usecs,
				  uint32_t  tics,
				  uint32_t	priority)
{
	uint32_t					loadval;

	if  (chnl > 3)  return  0;

	if (usecs != 0)
	{
		loadval =  (periph_clk_khz / 1000) * usecs;
	}
	else if (tics != 0)
	{
		loadval = tics;
	}
	else  return  0;

	if (priority > 15)  priority = 15;	// if illegal priority, use default instead

	pisr[chnl] = isrhandler;

	SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;
	PIT_MCR = 0;						// turn on PIT
	PIT_LDVAL(chnl) = loadval;			// store the reload value
	PIT_TCTRL(chnl) = PIT_TCTRL_TIE_MASK;	// enable PIT interrupts for this channel
	PIT_TCTRL(chnl) |= PIT_TCTRL_TEN_MASK;	// now start the timer channel

/*
 *  Update NVIC to handle PIT interrupts, based on selected channel
 *  (Refer to K20 Reference Manual and K20 Quick Reference Guide from Freescale)
 */
	switch  (chnl)
	{
/*
 *  PIT interrupt, channel 0
 *  Vector = 84
 *  IRQ = 68
 *  PIT0 bit = IRQ mod 32 = 68 mod 32 = 4
 *  NVIC register offset (NVICSERx, etc) = IRQ / 32 = 68 / 32 = 2
 *  Priority is 0-15 (0 is highest), written to high four bits of NVICIP68.
 *
 */
		case  0:				// PIT0
		NVICICPR2 |= (1<<4);	// clear any pending interrupt
		NVICISER2 |= (1<<4);	// enable PIT0 interrupt
		NVICIP68 = priority << 4;		// set priority level for this IRQ to (pppp 0000)
		break;

/*
 *  PIT interrupt, channel 1
 *  Vector = 85
 *  IRQ = 69
 *  PIT0 bit = IRQ mod 32 = 69 mod 32 = 5
 *  NVIC register offset (NVICSERx, etc) = IRQ / 32 = 69 / 32 = 2
 *  Priority is 0-15 (0 is highest), written to high four bits of NVICIP69.
 *
 */
		case  1:				// PIT1
		NVICICPR2 |= (1<<5);	// clear any pending interrupt
		NVICISER2 |= (1<<5);	// enable PIT1 interrupt
		NVICIP69 = priority << 4;		// set priority level for this IRQ to (pppp 0000)
		break;

/*
 *  PIT interrupt, channel 2
 *  Vector = 86
 *  IRQ = 70
 *  PIT0 bit = IRQ mod 32 = 70 mod 32 = 6
 *  NVIC register offset (NVICSERx, etc) = IRQ / 32 = 70 / 32 = 2
 *  Priority is 0-15 (0 is highest), written to high four bits of NVICIP70.
 *
 */
		case  2:				// PIT2
		NVICICPR2 |= (1<<6);	// clear any pending interrupt
		NVICISER2 |= (1<<6);	// enable PIT2 interrupt
		NVICIP70 = priority << 4;		// set priority level for this IRQ to (pppp 0000)
		break;

/*
 *  PIT interrupt, channel 3
 *  Vector = 87
 *  IRQ = 71
 *  PIT0 bit = IRQ mod 32 = 71 mod 32 = 7
 *  NVIC register offset (NVICSERx, etc) = IRQ / 32 = 71 / 32 = 2
 *  Priority is 0-15 (0 is highest), written to high four bits of NVICIP71.
 *
 */
		case  3:				// PIT3
		NVICICPR2 |= (1<<7);	// clear any pending interrupt
		NVICISER2 |= (1<<7);	// enable PIT3 interrupt
		NVICIP71 = priority << 4;		// set priority level for this IRQ to (pppp 0000)
		break;

		default:				// better never happen!
		return  0;
	}
	return  loadval;
}



void  PITStop(uint32_t  chnl)
{
	if  (chnl <= 3)  PIT_TCTRL(chnl) &= ~(PIT_TCTRL_TEN_MASK);	// disable the selected PIT channel
}


void  PITStart(uint32_t  chnl)
{
	if  (chnl <= 3)  PIT_TCTRL(chnl) |= PIT_TCTRL_TEN_MASK;	// enable the selected PIT channel
}


/*
 *  These are the IRQ handlers for the PIT interrupts.
 *
 *  These routines overwrite the default handlers found in crt0.s.  Those
 *  handler routines were all declared as weak, so the following labels
 *  do not create a multiply-defined linker error.
 *
 *  Each routine first clears the interrupt flag to disable the interrupt,
 *  then invokes the user-defined callback function (found in array pisr[]).
 *  The callback function is passed an identifier for the PIT channel that
 *  caused the interrupt.
 */
void  PIT0_IRQHandler (void)
{
	PIT_TFLG(0) = PIT_TFLG_TIF_MASK;	// always clear interrupt flag first!
	if (pisr[0])  (*pisr[0])(0);
}

void  PIT1_IRQHandler (void)
{
	PIT_TFLG(1) = PIT_TFLG_TIF_MASK;	// always clear interrupt flag first!
	if (pisr[1])  (*pisr[1])(1);
}

void  PIT2_IRQHandler (void)
{
	PIT_TFLG(2) = PIT_TFLG_TIF_MASK;	// always clear interrupt flag first!
	if (pisr[2])  (*pisr[2])(2);
}

void  PIT3_IRQHandler (void)
{
	PIT_TFLG(3) = PIT_TFLG_TIF_MASK;	// always clear interrupt flag first!
	if (pisr[3])  (*pisr[3])(3);
}

