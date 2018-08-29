/*
 * main.c
 *
 *  Created on: 18.04.2018
 *      Author: conradwild
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/twi.h>
#include "twislave.h"

#define WAIT_CYCLES 10

#define FALSE 0
#define TRUE 1

volatile uint8_t go_to_sleep = TRUE;
volatile uint8_t cycle_cnt;
volatile uint16_t int0_cnt;

//prescaler clk/1014
#define start_timer(t_val) \
	TCNT0 = 0; \
	OCR0A = t_val; \
	TCCR0B |= (1 << CS02) | (1 << CS00); \
	go_to_sleep = FALSE;

//set cs02:0 to '0'
#define stop_timer \
	TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00)); \
	TCNT0 = 0; \
	go_to_sleep = TRUE;

const int CSleep2Seconds = 0b00000110;

ISR(INT0_vect, ISR_BLOCK) {
	PORTB |= (1 << PB0);
	start_timer(0x0a)
	;
//	reti();
}

ISR(TIM0_COMPA_vect, ISR_BLOCK) {
	PORTB &= ~(1 << PB0);
	stop_timer
	;
//	reti();
}

ISR(WDT_vect, ISR_BLOCK) {
	WDTCR |= (1 << WDIE);
	++cycle_cnt;
	if (cycle_cnt > WAIT_CYCLES) {
		PORTB |= (1 << PB0);
		start_timer(0x0a)
		;
		cycle_cnt = 0;
	}
}

void start_watchdog(int interval) {
	wdt_reset();

	// This order of commands is important and cannot be combined
	MCUSR = 0;                   // reset Brown-out, Ext and PowerOn reset flags
	WDTCR |= 0b00011000; // see docs, set (WDCE -> next set prescaler), (WDE -> Interrupt on timeout)
	WDTCR = 0b01000000 | interval;     // set WDIE, and appropriate delay
	ADCSRA &= ~_BV(ADEN);
}

void chip_setup() {
	cli();

	DDRB = 0;
	DDRB |= (1 << PB0);
	PORTB = 0;
	//pull-up
	PORTB |= (1 << PB1);
	//interrupt on compare match a
	TIMSK |= (1 << OCIE0A);
	OCR0A = 0xa0;
//	int0 bei fallender flanke
	MCUCR |= (1 << ISC01);
	GIMSK |= (1 << INT0);

	set_sleep_mode(SLEEP_MODE_IDLE);

//	start_watchdog(CSleep2Seconds);

	sei();
}

void do_sleep() {
	if (go_to_sleep == TRUE) {
		sleep_bod_disable()
		;
		sleep_mode()
		;    // Now enter sleep mode
	}
}

int main(void) {
	chip_setup();
	while (TRUE) {
		do_sleep();
	}
	return 0;
}

