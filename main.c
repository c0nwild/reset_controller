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
	TCCR0A |= (1 << CS02) | (1 << CS00); \
	go_to_sleep = FALSE;

#define start_timer_refrac \
	TCNT1 = 0; \
	TCCR1B |= (1 << CS12) | (1 << CS10);

//set cs02:0 to '0'
#define stop_timer \
	TCCR0A &= ~((1 << CS02) | (1 << CS01) | (1 << CS00)); \
	TCNT0 = 0; \
	go_to_sleep = TRUE; \
	start_timer_refrac

#define stop_timer_refrac \
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); \
	TCNT1 = 0;

const int CSleep2Seconds = 0b00000110;

ISR(INT0_vect, ISR_BLOCK) {
	PORTB |= (1 << PB0);
	start_timer(0x8a);
	EIMSK &= ~(1 << INT0); //erst mal keine INT0 interrupts mehr
	EIFR |= (1 << INTF0);
}

ISR(TIMER0_COMPA_vect, ISR_BLOCK) {
	PORTB &= ~(1 << PB0);
	stop_timer
	;
}

ISR(TIMER1_COMPB_vect, ISR_BLOCK) {
	EIMSK |= (1 << INT0);
	stop_timer_refrac
	;
}

//ISR(WDT_vect, ISR_BLOCK) {
//	WDTCSR |= (1 << WDIE);
//	++cycle_cnt;
//	if (cycle_cnt > WAIT_CYCLES) {
//		PORTB |= (1 << PB0);
//		start_timer(0x0a)
//		;
//		cycle_cnt = 0;
//	}
//}

//void start_watchdog(int interval) {
//	wdt_reset();
//
//	// This order of commands is important and cannot be combined
//	MCUSR = 0;                   // reset Brown-out, Ext and PowerOn reset flags
//	WDTCSR |= 0b00011000; // see docs, set (WDCE -> next set prescaler), (WDE -> Interrupt on timeout)
//	WDTCSR = 0b01000000 | interval;     // set WDIE, and appropriate delay
//	ADCSRA &= ~_BV(ADEN);
//}

void chip_setup() {
	cli();

	DDRB |= (1 << DDB0);
	//pull-up
	PORTD |= (1 << PD2);
	//interrupt on compare match a
	TIMSK0 |= (1 << OCIE0A);
	//interrupt on compare match timer 1 a fuer refraktaerzeit
	TIMSK1 |= (1 << OCIE1B);
	OCR1B = 0xff;

//	int0 bei fallender flanke
//	EICRA |= (1 << ISC01); TEST: Interrupt bei low level
	EIMSK |= (1 << INT0);
	EIFR |= (1 << INTF0);
	sei();
}

void do_sleep() {
	if (go_to_sleep == TRUE) {
		sleep_bod_disable()
		;
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode()
		;    // Now enter sleep mode
	}
}

int main(void) {
	chip_setup();
	while (TRUE) {
//		do_sleep();
	}
	return 0;
}

