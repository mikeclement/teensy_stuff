/*
 *  pit.h      header file for Periodic Interrupt Timer module (libpit.a)
 *
 *  This header defines functions available to support PIT use on the
 *  Teensy 3.1.
 */

#ifndef  PIT_H
#define  PIT_H


/*
 *           Guidelines for using the PIT library
 *
 *  You create a PIT timer by invoking PITInit with a chennel
 *  number, a pointer to an ISR callback function, a delay
 *  value, and a priority.
 *
 *  You have two options for specifying the timer delay.
 *  To specify the value in microseconds, invoke PITInit with
 *  a value for the second argument (usecs).  To specify the
 *  value in peripheral clocks, invoke PITInit with a value
 *  for the third argument.  Whichever argument you DON'T need
 *  to use should be passed as 0.
 *
 *  Calling PITInit sets up and starts the PIT timer.
 *
 *  For example:
 *
 *    PITInit(0, MyHandler, 1000000, 0, 3);     // assign a 1-sec delay to timer 0, priority 3
 *    PITInit(1, MyHandler, 0, 1234, 3);        // delay 1234 peripheral clocks for timer 1, pri 3
 *
 *  You can call PITStop to halt a PIT timer.
 *
 *  You can call PITStart to restart a PIT timer.
 *
 *  You must supply a callback function when you initialize a PIT.
 *  This callback function will be called by the PIT IRQ at each
 *  interrupt.  Your callback function does not need to clear the
 *  PIT interrupt flag; that will have already been done.  The callback
 *  function will be passed an argument containing the PIT channel that
 *  caused the interrupt.
 */



/*
 *  PITInit      initialize one of the PIT timer channels
 *
 *  This routine turns on the PIT module and enables a selected PIT
 *  timer.
 *
 *  Argument chnl selects one of the four PIT timers; chnl must be
 *  a value from 0 through 3.  Argument isrhandler points to a function
 *  that returns a void and accepts a single char as its argument.
 *  Argument usecs, if not zero, sets the number of microseconds per
 *  timer interrupt.  Argument tics, if not zero, sets the number of
 *  peripheral clocks per timer interrupt.
 *
 *  Upon entry, either argument usecs or argument tics must be non-zero.
 *  Use usecs if your desired interrupt can use usec resolution.  If you
 *  need greater resolution, use 0 for usecs and set the desired number
 *  of peripheral clock tics using argument tics.
 *
 *  Upon entry, argument isrhandler points to an ISR function in your code
 *  that will service the PIT timer interrupt for the selected
 *  channel.  Your function must return a void and must accept a
 *  single argument, which will be a char containing the PIT timer
 *  number receiving the interrupt.
 *
 *  Upon entry, priority contains a value from 0 to 15, assigning the
 *  priority of the associated interrupt; a value of 0 assigns the
 *  highest level of priority.  If the value passed is out of this
 *  range, a value of 15 will be used.
 *
 *  Your ISR function can determine which PIT timer caused the interrupt
 *  by checking the argument passed in by the PIT library.  This
 *  lets you write a common PIT ISR handler, using the char argument
 *  to identify the interrupt source
 *
 *  The PIT uses the peripheral clock for timing.  You can determine
 *  the peripheral clock frequency at run-time using global variable
 *  periph_clk_khz.
 *
 *  Upon exit, this routine returns the PIT reload value for this
 *  channel upon success; if an error was detected, this routine
 *  returns 0.
 */
uint32_t  PITInit(uint32_t  chnl,
				  void (*isrhandler)(char),
				  uint32_t  usecs,
				  uint32_t  tics,
				  uint32_t	priority);

/*
 *  PITStop      disable (stop) a selected PIT channel
 *
 *  This routine disables a PIT channel by clearing the TEN bit
 *  in the timer's TCTRL register.  The reload value is not disturbed.
 *
 *  Argument chnl must be a PIT channel number (0 through 3).  The
 *  selected channel must have been initialized with an earlier call
 *  to PITInit.  Do not call this routine to stop a PIT timer that
 *  was not previously initialized!
 */
void			PITStop(uint32_t  chnl);


/*
 *  PITStart      reenable (start) a selected PIT channel
 *
 *  This routine reenables a PIT channel previously stopped by the
 *  PITStop routine.
 *
 *  Argument chnl must be a PIT channel number (0 through 3).  The
 *  selected channel must have been initialized with an earlier call
 *  to PITInit.  Do not call this routine to start a PIT timer that
 *  was not previously initialized!
 */

void			PITStart(uint32_t  chnl);


#endif

