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

#define INT_SRC_LB_PULSE 0x1
#define INT_SRC_ESP_PULSE 0x2

volatile uint8_t go_to_sleep = TRUE;
volatile uint8_t interrupt_src = 0;
volatile uint8_t do_eval_interrupt = FALSE;
//volatile uint16_t int0_cnt;

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
	start_timer_refrac

#define stop_timer_refrac \
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); \
	TCNT1 = 0;

ISR(INT0_vect, ISR_BLOCK) {
	EIMSK &= ~(1 << INT0); //erst mal keine INT0 interrupts mehr
	EIFR |= (1 << INTF0);
	do_eval_interrupt = TRUE;
}

ISR(INT1_vect, ISR_BLOCK) {
	EIMSK &= ~(1 << INT1); //erst mal keine INT1 interrupts mehr
	EIFR |= (1 << INTF1);
	do_eval_interrupt = TRUE;
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

void chip_setup() {
	cli();

	DDRB |= (1 << DDB0);
	//pull-up int0 und int1
	PORTD |= (1 << PD2) | (1 << PD3);
	//interrupt on compare match a
	TIMSK0 |= (1 << OCIE0A);
	//interrupt on compare match timer 1 a fuer refraktaerzeit
	TIMSK1 |= (1 << OCIE1B);
	OCR1B = 0xff;

//	int0/int1 bei fallender flanke
//	EICRA |= (1 << ISC01); TEST: Interrupt bei low level
	EIMSK |= (1 << INT0) | (1 << INT1);
	EIFR |= (1 << INTF0);
	sei();
}

void eval_interrupt(void) {
	interrupt_src = 0;
	interrupt_src |= ((PIND & PIND2) ? INT_SRC_LB_PULSE : 0);
	interrupt_src |= ((PIND & PIND3) ? INT_SRC_ESP_PULSE : 0);
	do_eval_interrupt = FALSE;
}

void do_master_wakeup(void) {
		PORTB |= (1 << PB0);
		start_timer(0x8a);
}

void do_sleep() {
	if (go_to_sleep == TRUE) {
		sleep_bod_disable()
		;
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_mode()
		;    // Now enter sleep mode
	}
	// aufgewacht. Rausfinden, warum und master wecken.
	if (do_eval_interrupt == TRUE){
		eval_interrupt();
		do_master_wakeup();
	}
}

int main(void) {
	chip_setup();
	init_twi_slave(0x90);
	while (TRUE) {
		do_sleep();
	}
	return 0;
}

