/*
 * buttons.c
 *
 * Author: Peter Sutton
 */ 

#include "buttons.h"
#include "timer0.h"
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Global variable to keep track of the last button state so that we 
// can detect changes when an interrupt fires. The lower 4 bits (0 to 3)
// will correspond to the last state of port B pins 0 to 3.
static uint8_t last_button_state;
static volatile uint8_t button_state;

// Our button queue. button_queue[0] is always the head of the queue. If we
// take something off the queue we just move everything else along. We don't
// use a circular buffer since it is usually expected that the queue is very
// short. In most uses it will never have more than 1 element at a time.
// This button queue can be changed by the interrupt handler below so we should
// turn off interrupts if we're changing the queue outside the handler.
#define BUTTON_QUEUE_SIZE 4
static volatile int8_t button_queue[BUTTON_QUEUE_SIZE];
//static volatile uint32_t held_queue_time[BUTTON_QUEUE_SIZE];
static volatile uint32_t held_queue[BUTTON_QUEUE_SIZE];
//static volatile uint8_t remove_queue[BUTTON_QUEUE_SIZE];
//static volatile int8_t held_queue_length;
static volatile int8_t button_queue_length;
//static volatile int8_t remove_queue_length;
static volatile uint8_t held_buttons;
static volatile uint32_t debouce;

// Setup interrupt if any of pins B0 to B3 change. We do this
// using a pin change interrupt. These pins correspond to pin
// change interrupts PCINT8 to PCINT11 which are covered by
// Pin change interrupt 1.
void init_button_interrupts(void) {
	// Enable the interrupt (see datasheet page 77)
	PCICR |= (1 << PCIE1);
	
	// Make sure the interrupt flag is cleared (by writing a 
	// 1 to it) (see datasheet page 78)
	PCIFR |= (1 << PCIF1);
	
	// Choose which pins we're interested in by setting
	// the relevant bits in the mask register (see datasheet page 78)
	PCMSK1 |= (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);	
	
	// Empty the button push queue
	button_queue_length = 0;
	//held_queue_length = 0;
	//remove_queue_length = 0;
}

static volatile uint32_t cur_time = 0;

int8_t button_pushed(void) {
	int8_t return_value = NO_BUTTON_PUSHED;	// Assume no button pushed
	
	cur_time = get_current_time();
	
	if (button_queue_length > 0) {
		// Remove the first element off the queue and move all the other
		// entries closer to the front of the queue. We turn off interrupts (if on)
		// before we make any changes to the queue. If interrupts were on
		// we turn them back on when done.
		uint8_t pin = button_queue[0];
		return_value = (1 << pin);
		held_queue[pin] = cur_time + INITIAL_HOLD;
			
		// Save whether interrupts were enabled and turn them off
		int8_t interrupts_were_enabled = bit_is_set(SREG, SREG_I);
		cli();
			
		for (uint8_t i = 1; i < button_queue_length; i++) {
			button_queue[i - 1] = button_queue[i];
		}
		button_queue_length--;
			
		if (interrupts_were_enabled) {
			// Turn them back on again
			sei();
		}
	}

	//TODO: This feels hacky maybe theres a better way to do this
	for (uint8_t pin = 0; pin < NUM_BUTTONS; pin++) {
		if(held_queue[pin] && (last_button_state & (1 << pin))){
			// Button is being held
			uint32_t held_time = held_queue[pin];
			if(cur_time >= held_time){
				return_value |= (1 << pin);
				held_queue[pin] = cur_time + HOLD_ACTION_DELAY;
			}
		}else if(held_queue[pin] && !(last_button_state & (1<< pin))){
			held_queue[pin] = 0;
		}
	}

	return return_value;
}

// Interrupt handler for a change on buttons
ISR(PCINT1_vect) {
	// Get the current state of the buttons. We'll compare this with
	// the last state to see what has changed.
	button_state = PINB & 0x0F;
	
	cur_time = get_current_time();
	
	// Iterate over all the buttons and see which ones have changed.
	// Any button pushes are added to the queue of button pushes (if
	// there is space). We ignore button releases so we're just looking
	// for a transition from 0 in the last_button_state bit to a 1 in the 
	// button_state.
	for (uint8_t pin = 0; pin < NUM_BUTTONS; pin++) {
		if (button_queue_length < BUTTON_QUEUE_SIZE
		&& (button_state & (1 << pin))
		&& !(last_button_state & (1 << pin))) {
			// Add the button push to the queue (and update the
			// length of the queue
			button_queue[button_queue_length++] = pin;
		}
		//else if(!(button_state & (1<<pin))
		//		&& (last_button_state & (1 << pin))){
		// Add no longer held buttons to the remove queue
		//	remove_queue[remove_queue_length++] = (1<<pin);
		//	printf("RELEASED");
		//}
	}
	
	// Remember this button state
	last_button_state = button_state;
}
