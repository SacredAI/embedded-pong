/*
 * sound.c
 *
 * Created: 19/05/2023 6:03:53 PM
 *  Author: AlexD
 */ 

#include "sound.h"
#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "tunes.h"
#include "timer0.h"

#define F_CPU 8000000UL
#include <util/delay.h>

/*static uint16_t freq = 200; // Hz*/
/*static float duty_cycle = 50; // %*/
static char ToneOn = 0;
volatile char tunes_playing = 0;
static volatile char Volume = 50;
static int *CurrentSong;
static uint8_t last_tempo;
static unsigned int Wholenote;
uint16_t delay_ms = 0;

static uint8_t mute = 0;

// For a given frequency (Hz), return the clock period (in terms of the
// number of clock cycles of a 1MHz clock)
uint16_t freq_to_clock_period(uint16_t freq) {
	return (1000000UL / freq);	// UL makes the constant an unsigned long (32 bits)
	// and ensures we do 32 bit arithmetic, not 16
}

// Return the width of a pulse (in clock cycles) given a duty cycle (%) and
// the period of the clock (measured in clock cycles)
uint16_t duty_cycle_to_pulse_width(float dutycycle, uint16_t clockperiod) {
	return (dutycycle * clockperiod) / 100;
}

void toggle_mute(void){
	mute ^= 1;
	if (mute)
	{
		Tunes_Stop();
	}
}

void Tunes_Play_Mario(void){
	Tunes_Play(mario_main_theme, 120);
}

void Tunes_Play_star(void){
	Tunes_Play(star_wars_theme, 108);
}

void setup_sound_effects(void){
	
	// Set up timer/counter 1 for Fast PWM, counting from 0 to the value in OCR1A
	// before reseting to 0. Count at 1MHz (CLK/8).
	// Configure output OC1B to be clear on compare match and set on timer/counter
	// overflow (non-inverting mode).
	TCCR1A = (1<<WGM10);
	TCCR1B = (1<<WGM13);
	
	Tunes_SetTimer();
}

void Tone(uint16_t tone, uint16_t duration_ms){
	if(mute) return;
	tunes_playing = 1;
	
	uint16_t clockperiod = freq_to_clock_period(tone);
	uint16_t pulsewidth = duty_cycle_to_pulse_width(Volume, clockperiod);
	
	OCR1A = clockperiod - 1;
	
	if(pulsewidth == 0){
		OCR1B = 0;
	}else{
		OCR1B = pulsewidth - 1;
	}
	
	Tunes_SetTimer();
	
	if(duration_ms){
		delay_ms = get_current_time() + duration_ms;
		ToneOn = 1;
	}
}

void Tunes_SetTimer(void){
	// Make pin OC1B (D4) be an output
	DDRD |= (1<<4);

	TCCR1A |= (1<<COM1B1);
	TCCR1B |= (1<<CS11);
}

void Tunes_Play(const int *pMusicNotes, uint8_t tempo){
	if(tunes_playing == 0){
		tunes_playing = 1;
		
		// Save current song and tempo
		CurrentSong = (int *)pMusicNotes;
		
		// Calculate the duration of a whole note in ms (60s/tempo)*4 beats
		Wholenote = ((uint32_t)60000 * 4) / tempo;
		
		last_tempo = tempo;
		
		Tunes_SetTimer();
	}
}

void Tunes_Stop(void){
	TCCR1B &= ~(1<<CS11);
	
	TCCR1A &= ~((1<<COM1B1));
	
	OCR1A = 0;
	OCR1B = 0;
	TCNT1 = 0;
	
	DDRD &= ~(1<<4);
	
	tunes_playing = 0;
}

void Tunes_SetVolume(uint8_t volume){
	if(volume > 50) volume = 50;
	Volume = volume;
}

char Tunes_IsPlaying(void){
	return tunes_playing;
}

void Tunes_alert_alarm(uint8_t vuvuzela){
	uint16_t f;
	
	for(f = 100; f < 1000; f++){
			_delay_ms(1);
			
			if(vuvuzela == 0) while(TCNT1 > 0);
			
			Tone(f, 0);
			
			if(vuvuzela) TCNT1 = 0;
	}
	
	Tunes_Stop();
}

void Tunes_Think(void){
	if(mute) return;
	static int16_t duration = 0;
	uint16_t note = 0;
	
	if(delay_ms < get_current_time()){
		if(ToneOn){
			ToneOn = 0;
			Tunes_Stop();
			return;
		}
		
		if(pgm_read_word(CurrentSong)){
			note = pgm_read_word(CurrentSong);
			
			CurrentSong++;
			
			if((int16_t)pgm_read_word(CurrentSong) < 33 && (int16_t)pgm_read_word(CurrentSong) != REST && (int16_t)pgm_read_word(CurrentSong) != 0){
				duration = (int16_t)pgm_read_word(CurrentSong);
				
				if(duration > 0){
					duration = Wholenote / duration;
				}else{
					duration = Wholenote / (0 - duration);
					duration = duration + (duration / 2);
				}
				
				CurrentSong++;
			}
			
			if(note == REST){
				OCR1A = 0;
			}else{
				uint16_t clockperiod = freq_to_clock_period(note);
				uint16_t pulsewidth = duty_cycle_to_pulse_width(Volume, clockperiod);
				
				OCR1A = clockperiod - 1;
				
				if(pulsewidth == 0){
					OCR1B = 0;
				}else{
					OCR1B = pulsewidth - 1;
				}
				
				TCNT1 = 0;
			}
			
			delay_ms = get_current_time() + duration;
		}else{
			Tunes_Stop();
		}
	}
}