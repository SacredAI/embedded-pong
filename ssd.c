/*
 * ssd.c
 *
 * Created: 11/05/2023 6:24:07 PM
 *  Author: AlexD
 */ 

#include "ssd.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "game.h"

//  Seven Segment display from 0 to F
uint8_t seven_seg[16] = { 63,6,91,79,102,109,125,7,127,111,119,124,57,94,121,113};

/* digits_displayed - 1 if digits are displayed on the seven
** segment display, 0 if not. No digits displayed initially.
*/
volatile uint8_t digits_displayed = 0;

/* Seven segment display digit being displayed.
** 0 = right digit; 1 = left digit.
*/
volatile uint8_t seven_seg_cc = 0;


void setup_ssd(){
	// Set port C (all pins) to outputs
	DDRC = 0xFF;
	
	// Set Port D pin 2 to output
	DDRD |= (1<<DDD2);
	
	/* Set up timer/counter 2 so that we get an 
	** interrupt 100 times per second, i.e. every
	** 10 milliseconds.
	*/
	OCR2A = 77; /* Clock divided by 1024 - count for 78 cycles */
	TCCR2A = (1<<WGM21); /* CTC Mode */
	
	TCCR2B = (1<<CS22)|(1<<CS21)|(1<<CS20); /* Divide clock by 1024 */
	
	TIMSK2 |= (1<<OCIE2A);
	
	/* Make sure the interrupt flag is cleared by writing a 
	 * 1 to it.
	 */
	TIFR2 = (1 << OCF2A);
}

void ssd_display_score(){
	digits_displayed = 1;
}

ISR(TIMER2_COMPA_vect){
	
	/* Change which digit will be displayed. If last time was
	** left, now display right. If last time was right, now 
	** display left.
	*/
	seven_seg_cc = 1 ^ seven_seg_cc;
	
	
	if(digits_displayed){
		int8_t score = 0;
		if(seven_seg_cc == 0){
			score = get_player_score(PLAYER_2);
		}else{
			score = get_player_score(PLAYER_1);
		}
		PORTC = seven_seg[score];
		// Set pin D2 to seven_seg_cc
		PORTD = (PORTD & ~(1<<PORTD2)) | (seven_seg_cc<<PORTD2);
	}else{
		/* No digits displayed -  display is blank */
		PORTC = 0;
	}
}