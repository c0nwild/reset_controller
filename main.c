/*
 * main.c
 *
 *  Created on: 18.04.2018
 *      Author: conradwild
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define FALSE 0
#define TRUE 1

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


ISR(INT0_vect, ISR_BLOCK) {
	PORTB |= (1 << PB0);
	start_timer;
	reti();
}

ISR(TIM0_COMPA_vect, ISR_BLOCK) {
	PORTB &= ~(1 << PB0);
	stop_timer;
	reti();
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
	//int0 bei fallender flanke
	MCUCR |= (1 << ISC01);
	GIMSK |= (1 << INT0);
	sei();
}

void do_sleep() {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	//nur falls wir eine Flanke verpasst haben
	//sleep_mode();
}

int main(void) {
	chip_setup();
	while (TRUE) {
		if (go_to_sleep == TRUE)
			do_sleep();
	}
	return 0;
}

