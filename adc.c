/*
 * adc.c
 *
 * Created: 14/05/2023 6:50:10 PM
 *  Author: Alex Donnellan
 */ 

#include "adc.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "game.h"
#include "timer0.h"
#include "buttons.h"

#include "serialio.h"
#include <stdio.h>

#define ADC_QUEUE_SIZE 4
static volatile uint16_t adc_queue[ADC_QUEUE_SIZE];
static volatile uint8_t adc_queue_length;

// Setup interrupts on adc conversion complete and adc autotrigger
void init_adc_interrupts(void) {
	// Set up ADC - AVCC reference, right adjust
	// Input ADC0
	ADMUX = (1<<REFS0);
	
	// Turn on the ADC (but don't start a conversion yet). Choose a clock
	// divider of 64. (The ADC clock must be somewhere
	// between 50kHz and 200kHz. We will divide our 8MHz clock by 64
	// to give us 125kHz.)
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
	
	// Enable Interrupt
	ADCSRA |= (1<<ADIE);
	
	// Make sure interrupt flag is clear
	ADCSRA |= (1<<ADIF);

}

uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static uint32_t cur_time = 0, next_read_time = 0, next_queue_time = 0;
static uint16_t last_mapped = 0;
static uint8_t holding = 0;
//
int8_t adc_move(void){
	// Ensure we're always trying to read the adc value
	cur_time = get_current_time();
	if(cur_time >= next_read_time){
		ADCSRA |= (1<<ADSC);
		// Try read every 50 ms
		next_read_time = cur_time + 50;
	}
	
	int8_t return_value = NO_MOVEMENT;
	
	if(adc_queue_length > 0 ){
		uint16_t val = adc_queue[0];
		uint16_t mapped = map(val, 0, 1024, 0, 3) - 1;
		/*printf("X: %4d, Mapped: %4d \n", val, mapped);*/
		if(mapped != last_mapped){
			holding = 0;
			next_queue_time = 0;
		}else if(!holding){
			holding = 1;
			next_queue_time = cur_time + INITIAL_HOLD;
		}
		if(!next_queue_time || cur_time >= next_queue_time){
			switch(mapped){
				case -1:
				return_value = PLAYER_2_DOWN;
				break;
				case 1:
				return_value = PLAYER_2_UP;
				break;
			}
			if(holding){
				next_queue_time = cur_time + HOLD_ACTION_DELAY;
			}
		}
		
		// Save whether interrupts were enabled and turn them off
		int8_t interrupts_were_enabled = bit_is_set(SREG, SREG_I);
		cli();
		
		for (uint8_t i = 1; i < adc_queue_length; i++) {
			adc_queue[i - 1] = adc_queue[i];
		}
		adc_queue_length--;
		
		if (interrupts_were_enabled) {
			// Turn them back on again
			sei();
		}
		
		last_mapped = mapped;
	}
	
	return return_value;
}

// Interrupt handler for ADC Conversion
ISR(ADC_vect){
	
	uint16_t adc_state = ADC;
	
	if(adc_queue_length < ADC_QUEUE_SIZE){
		adc_queue[adc_queue_length++] = adc_state;
	}
}