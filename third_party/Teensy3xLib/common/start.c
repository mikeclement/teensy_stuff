/****************************************************************************************************/
/**
\file       start.c
\brief      kinetis startup routines
\author     Freescale Semiconductor
\author     
\version    1.0
\date       Sep 14, 2011
\warning    need to check with Si
 
*/
#include "arm_cm4.h"
#include "common.h"
#include "start.h"
#include "wdog.h"
#include "sysinit.h"

/********************************************************************/
/*!
 * \brief   Kinetis Start
 * \return  None
 *
 * This function calls all of the needed starup routines and then 
 * branches to the main process.
 *
 * NOTE: Prior to entry, the MCU's watchdog MUST be disabled!  This
 * is typically done in crt0.s.  If the watchdog is not disabled
 * after reset, you will get a watchdog interrupt 256 cycles after
 * reset.
 */
void start(void)
{
/*
 * Enable all of the port clocks. These have to be enabled to configure
 * pin muxing options, so most code will need all of these on anyway.
 */
        SIM_SCGC5 |= (SIM_SCGC5_PORTA_MASK
                    | SIM_SCGC5_PORTB_MASK
                    | SIM_SCGC5_PORTC_MASK
                    | SIM_SCGC5_PORTD_MASK
                    | SIM_SCGC5_PORTE_MASK );

	/* Perform processor initialization */
	sysinit();

	/* Jump to main process */
	main();

	/* No actions to perform after this so wait forever */
	while (1)   ;
}

