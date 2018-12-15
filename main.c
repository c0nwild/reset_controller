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
#include <stddef.h>
#include "twislave.h"

#define WAIT_CYCLES 10

#define FALSE 0u
#define TRUE 1u

//Quellen fuer INT's
#define INT_SRC_MCP_PULSE (1 << 0) //MCP23017
#define INT_SRC_ESP_PULSE (1 << 1) //ESP8266

//Init merker
#define STATUS_SYS_INITIALIZED (1 << 2) //Merker für erstes Init nach Poweroff

//Freigabe für sleep
#define STATUS_SYS_ALLOWRESET (1 << 3) //Freigabemerker für Reset, kommt vom ESP8266

volatile uint8_t go_to_sleep = FALSE;
volatile uint8_t interrupt_src = 0;
volatile uint8_t do_eval_interrupt = FALSE;
volatile uint8_t is_initialized = FALSE;
//volatile uint16_t int0_cnt;

const uint8_t i2c_addr = 0x47;

//prescaler clk/1014
#define start_timer(t_val) \
	TCNT0 = 0; \
	OCR0A = t_val; \
	TCCR0A |= (1 << CS02) | (1 << CS00); \

//set cs02:0 to '0'
#define stop_timer \
	TCCR0A &= ~((1 << CS02) | (1 << CS01) | (1 << CS00)); \
	TCNT0 = 0; \

//INT vom MCP23017
ISR(INT0_vect, ISR_BLOCK) {
	uint8_t i_src;
	i_src = interrupt_src;
	i_src |= INT_SRC_MCP_PULSE;
	EIMSK &= ~(1 << INT0); //erst mal keine INT0 interrupts mehr
	EIFR |= (1 << INTF0);
	do_eval_interrupt = TRUE;
	go_to_sleep = FALSE;
	interrupt_src = i_src;
}

// INT vom ESP8266
ISR(INT1_vect, ISR_BLOCK) {
	uint8_t i_src;
	i_src = interrupt_src;
	i_src |= INT_SRC_ESP_PULSE;
	EIMSK &= ~(1 << INT1); //erst mal keine INT1 interrupts mehr
	EIFR |= (1 << INTF1);
	do_eval_interrupt = TRUE;
	go_to_sleep = FALSE;
	interrupt_src = i_src;
}

ISR(TIMER0_COMPA_vect, ISR_BLOCK) {
	PORTB &= ~(1 << PB0);
	stop_timer
	;
//	EIMSK |= (1 << INT0) | (1 << INT1);
}

void chip_setup() {
	cli();

	DDRB |= (1 << DDB0);
	//pull-up int0 und int1
	PORTD |= (1 << PD2) | (1 << PD3);
	//interrupt on compare match a
	TIMSK0 |= (1 << OCIE0A);

//	Interrupt bei low level
	EIMSK |= (1 << INT0) | (1 << INT1);
	EIFR |= (1 << INTF0) | (1 << INTF1);
	sei();
}

void eval_interrupt(volatile uint8_t *i2c_buffer) {
	interrupt_src |= ((PIND & PIND2) ? INT_SRC_ESP_PULSE : 0);
	i2c_buffer[0] |= interrupt_src;
	i2c_buffer[1] &= ~interrupt_src;
	interrupt_src = 0;
	do_eval_interrupt = FALSE;
}

void do_master_wakeup(volatile uint8_t *i2c_buffer) {
	if (i2c_buffer[0] & STATUS_SYS_ALLOWRESET) {
		PORTB |= (1 << PB0);
		start_timer(0x20);
		//Verriegelung gegen erneuten reset
		i2c_buffer[0] &= ~(STATUS_SYS_ALLOWRESET);
		i2c_buffer[1] |= STATUS_SYS_ALLOWRESET;
	}
}

uint8_t allow_sleep(volatile uint8_t *i2c_buffer) {
	uint8_t rv = FALSE;
	if (i2c_buffer[0] & STATUS_SYS_ALLOWRESET)
		rv = TRUE;

	return rv;
}

void main_loop() {
	if (go_to_sleep == TRUE) {
		sleep_bod_disable()
		;
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		EIMSK |= (1 << INT0) | (1 << INT1);
		if (PIND & (1 << PD2)) {
			sleep_mode()
			;    // Now enter sleep mode
		}
	}
	// aufgewacht. Rausfinden, warum und master wecken.
	if (do_eval_interrupt == TRUE) {
		eval_interrupt(i2cdata);    //int quelle in i2cbuffer schreiben
		do_master_wakeup(i2cdata); //aufwecken, wenn freigabe erteilt
	}
	go_to_sleep = allow_sleep(i2cdata);
}

int main(void) {
	chip_setup();
	uint8_t addr = (i2c_addr << 1); //komisches Format (MSB 7:1 im TWAR enthalten Addresse!!)
	init_twi_slave(addr);

	i2cdata[1] = 0xff;

	while (TRUE) {
		main_loop();
	}
	return 0;
}

