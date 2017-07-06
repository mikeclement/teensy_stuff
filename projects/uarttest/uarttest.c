/*
 *  uarttest.c for the Teensy 3.1 board (K20 MCU, 16 MHz crystal)
 *
 *  This code will periodically send a character to the first UART
 *  Based on initialization/send code from Teensy3xLib
 */

#include  "common.h"

#define  LED_ON    GPIOC_PSOR=(1<<5)
#define  LED_OFF  GPIOC_PCOR=(1<<5)

// Timing parameters (relies on non-const, must init in main())
uint32_t hz, on_time, off_time;

// UART parameters
const UART_MemMapPtr uartbase = UART0_BASE_PTR;  // Set base address
const uint32_t baud = 9600;  // Set baudrate
const uint8_t ch = '.';  // Set character to emit

int  main(void)
{
  // Timing setup
  hz = (uint32_t)core_clk_khz * 100;
  on_time = hz / 10;
  off_time = hz - on_time;

  // LED setup
  PORTC_PCR5 = PORT_PCR_MUX(0x1); // LED is on PC5 (pin 13), config as GPIO (alt = 1)
  GPIOC_PDDR = (1<<5);            // make this an output pin
  LED_OFF;                        // start with LED off

  // UART setup
  // UARTs 0 and 1 use core, 2 uses periph
  uint32_t sysclk = core_clk_khz;
  // Enable clock for UART
  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
  // Disable UART while we change settings
  UART_C2_REG(uartbase) &= ~(UART_C2_TE_MASK    // disable transmitter
                            | UART_C2_RE_MASK    // disable receiver
                            | UART_C2_RIE_MASK);  // disable receive interrupt on buffer full
  // Default settings: 8-bit, no parity
  UART_C1_REG(uartbase) = 0;
  // Set baudrate
  register uint16_t sbr = (uint16_t)((sysclk * 1000)/(baud * 16));
  int8_t temp = UART_BDH_REG(uartbase) & ~(UART_BDH_SBR(0x1F));
  UART_BDH_REG(uartbase) = temp | UART_BDH_SBR(((sbr & 0x1F00) >> 8));
  UART_BDL_REG(uartbase) = (uint8_t)(sbr & UART_BDL_SBR_MASK);
  // Fine-tune baudrate using fractional divider
  register uint16_t  brfa = (((sysclk * 32000)/(baud * 16)) - (sbr * 32));
  temp = UART_C4_REG(uartbase) & ~(UART_C4_BRFA(0x1F));
  UART_C4_REG(uartbase) = temp |  UART_C4_BRFA(brfa);
  // Re-enable UART
  UART_C2_REG(uartbase) |= (UART_C2_TE_MASK
                          | UART_C2_RE_MASK
                          | UART_C2_RIE_MASK);
  // Connect UART to external pins
  PORTB_PCR17 = PORT_PCR_MUX(0x3);  // UART0 TXD is alt3 function on PB17
  PORTB_PCR16 = PORT_PCR_MUX(0x3);  // UART0 RXD is alt3 function on PB16
  // Update NVIC to handle receiver interrupts
  NVICICPR1 |= (1<<13);  // clear any pending interrupt
  NVICISER1 |= (1<<13);  // enable UART0 status source interrupt
  NVICIP45 = 0x30;       // set priority level for this IRQ to (pppp 0000)

  while (1)
  {
    volatile uint32_t n;

    // Turn LED on
    LED_ON;

    // Send a character across uart
    while (!(UART_S1_REG(uartbase) & UART_S1_TDRE_MASK));  // lock until ready
    UART_D_REG(uartbase) = ch;  // write char to UART

    // Wait a bit
    for (n=0; n<on_time; n++);

    // Turn LED off, wait remainder of second
    LED_OFF;
    for (n=0; n<off_time; n++);
  }

  return 0;  // should never get here!
}

void UART0_RX_TX_IRQHandler(void)
{
  char d;
  volatile uint32_t n;

  d = UART_S1_REG(uartbase);      // first part of clearing the interrupt
  if ((d & UART_S1_RDRF_MASK) == 0)      // if this is not a rcv interrupt...
    return;

  d = UART_D_REG(uartbase);        // get the received char
  LED_ON;
  while (!(UART_S1_REG(uartbase) & UART_S1_TDRE_MASK));  // lock until ready
  UART_D_REG(uartbase) = d;  // write char back to UART
  for (n=0; n<2000; n++);  // pause briefly so LED stays on
  LED_OFF;
}
