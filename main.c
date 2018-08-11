/*
 * main.c
 *
 *  Created on: 18.04.2018
 *      Author: conradwild
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define WAKEUP_INTERVAL 80
#define WAIT_CYCLES (WAKEUP_INTERVAL / 8)

#define FALSE 0
#define TRUE 1

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

volatile uint8_t go_to_sleep = TRUE;

//prescaler clk/1014
#define start_timer \
	TCCR0B |= (1 << CS02) | (1 << CS00); \
	go_to_sleep = FALSE;

//set cs02:0 to '0'
#define stop_timer \
	TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00)); \
	TCNT0 = 0; \
	go_to_sleep = TRUE;

static volatile unsigned cycle_count;

ISR(INT0_vect, ISR_BLOCK) {
	sbi(PORTB, PB0);
	start_timer
	;
	reti();
}

ISR(TIM0_COMPA_vect, ISR_BLOCK) {
	cbi(PORTB, PB0);
	stop_timer
	;
	reti();
}

ISR(WDT_vect) {
	cycle_count++;
	if (cycle_count > WAIT_CYCLES) {
		start_timer
		;
	}
}

// ii: 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=256ms, 5=500ms, 6=1s, 7=2s, 8=4s, 9=8s
void setup_watchdog(int ii) {
	unsigned char bb;
	int ww;
	if (ii > 9)
		ii = 9;
	bb = ii & 7;
	if (ii > 7)
		bb |= (1 << 5);
	bb |= (1 << WDCE);
	ww = bb;

	cbi(MCUSR, WDRF);
	sbi(WDTCR, WDCE);
	sbi(WDTCR, WDE);
	WDTCR = bb;
	sbi(WDTCR, WDIE);
}

void chip_setup() {
	cli();
	//watchdog
	setup_watchdog(9);
	DDRB = 0;
	sbi(DDRB, PB0);
	PORTB = 0;
	//pull-up
	sbi(PORTB, PB1);
	//interrupt on compare match a
	sbi(TIMSK, OCIE0A);
	OCR0A = 0xa0;
	//int0 bei fallender flanke
	sbi(MCUCR, ISC01);
	sbi(GIMSK, INT0);
	sei();
}

void do_sleep() {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	//nur falls wir eine Flanke verpasst haben
	sleep_mode();
}

int main(void) {
	chip_setup();
	while (TRUE) {
		if (go_to_sleep == TRUE)
			do_sleep();
	}
	return 0;
}

