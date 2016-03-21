#include  "common.h"

#define  LED_ON		GPIOC_PSOR=(1<<5)
#define  LED_OFF	GPIOC_PCOR=(1<<5)

#define DUTY_CYCLES								200
#define DUTY_PERIOD								1000
#define DUTY_PERCENT_MAX					40
#define DUTY_PERCENT_MAX_CYCLES		4000
#define DUTY_PERCENT_MIN					0
#define DUTY_PERCENT_MIN_CYCLES 	4000
#define DUTY_PERCENT_DELTA				2

void do_duty(const uint32_t cycles, const uint8_t percent) {
	uint32_t			c, d;
	uint32_t			duty;
	for (c=0; c<cycles; c++) {
		if (percent > 0) LED_ON;
		duty = DUTY_PERIOD * percent / 100;
		for (d=0; d<duty; d++);
		if (percent < 100) LED_OFF;
		duty = DUTY_PERIOD * (100 - percent) / 100;
		for (d=0; d<duty; d++);
	}
}

int main(void)
{
	int8_t				delta = DUTY_PERCENT_DELTA;
	int8_t				percent = DUTY_PERCENT_MIN;

	PORTC_PCR5 = PORT_PCR_MUX(0x1); // LED is on PC5 (pin 13), config as GPIO (alt = 1)
	GPIOC_PDDR = (1<<5);			// make this an output pin
	LED_OFF;						// start with LED off

	while (1)
	{
		do_duty(DUTY_CYCLES, (uint8_t)percent);

		percent += delta;

		if (percent > DUTY_PERCENT_MAX) {
			percent = DUTY_PERCENT_MAX;
			do_duty(DUTY_PERCENT_MAX_CYCLES, DUTY_PERCENT_MAX);
			delta = -delta;
		} else if (percent < DUTY_PERCENT_MIN) {
			percent = DUTY_PERCENT_MIN;
			do_duty(DUTY_PERCENT_MIN_CYCLES, DUTY_PERCENT_MIN);
			delta = -delta;
		}
	}

	return  0;						// should never get here!
}
