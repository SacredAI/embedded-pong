/*
 * project.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett, Cody Burnett
 * Modified by Alex Donnellan
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <ctype.h>


#define F_CPU 8000000UL
#include <util/delay.h>

#include "game.h"
#include "display.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"
#include "ssd.h"
#include "adc.h"
#include "cpu.h"
#include "sound.h"

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);
void draw_game_speed(int8_t speed);
void handle_serial_input(char input);
void handle_keyboard_movement(int8_t move);


/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete.
	start_screen();
	
	// Loop forever and continuously play the game.
	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	init_adc_interrupts();
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200, 0);
	
	init_timer0();
	
	// Seed random values
	srand(time(NULL));
	
	// Setup Seven Segment Display
	setup_ssd();
	
	//Buzzer
	setup_sound_effects();
	
	// Turn on global interrupts
	sei();
}

void start_screen(void) {
	// Clear terminal screen and output a message
	clear_terminal();
	show_cursor();
	move_terminal_cursor(10,10);
	printf_P(PSTR("PONG"));
	move_terminal_cursor(10,12);
	printf_P(PSTR("CSSE2010 A2 by Alex Donnellan - 46963037"));
	
	// Output the static start screen and wait for a push button 
	// to be pushed or a serial input of 's'
	show_start_screen();

	uint32_t last_screen_update, current_time;
	last_screen_update = get_current_time();
	
	uint8_t frame_number = 0;
	
	Tunes_Play_Mario();
	
	// Wait until a button is pressed, or 's' is pressed on the terminal
	while(1) {
		// First check for if a 's' is pressed
		// There are two steps to this
		// 1) collect any serial input (if available)
		// 2) check if the input is equal to the character 's'
		// If the serial input is 's', then exit the start screen
		char serial_input = get_serial_input();
		char lower_input = (char)tolower(serial_input);
		if (lower_input == 's') {
			break;
		}
		if(lower_input == 'm'){
			toggle_mute();
		}
		// Next check for any button presses
		int8_t btn = button_pushed();
		btn |= adc_move();
		if (btn != NO_BUTTON_PUSHED) {
			break;
		}

		current_time = get_current_time();
		if (current_time - last_screen_update > 500) {
			update_start_screen(frame_number);
			frame_number = (frame_number + 1) % 12;
			last_screen_update = current_time;
		}
		if(Tunes_IsPlaying()) Tunes_Think();
		if(!Tunes_IsPlaying()) Tunes_Play_Mario();
	}
	
	Tunes_Stop();
}

void new_game(void) {
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the game and display
	initialise_game();
	
	draw_game_speed(get_game_speed());
	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
	(void)adc_move();
}

uint32_t remaining_time, next_ball_move_time, current_time = 0;
int8_t btn; // The button pushed

void play_game(void) {
	char input; // Serial input
	
	next_ball_move_time = get_current_time();
	
	// We play the game until it's over
	while (!is_game_over()) {
				
		// We need to check if any button has been pushed, this will be
		// NO_BUTTON_PUSHED if no button has been pushed
		// Checkout the function comment in `buttons.h` and the implementation
		// in `buttons.c`.
		btn = button_pushed();
		
		btn |= adc_move();
		
		input = get_serial_input();
		
		handle_serial_input(input);
		
		handle_player_move(btn);
		
		cpu_think();
		guide_think();
		
		if(Tunes_IsPlaying()) Tunes_Think();
		
		current_time = get_current_time();
		if(is_game_paused()) continue;
		if (current_time >= next_ball_move_time) {
			// 500ms (0.5 second) has passed since the last time we move the
			// ball, so update the position of the ball based on current x
			// direction and y direction
			update_ball_position();
			
			// set the next move time for the ball 
			next_ball_move_time = current_time + get_game_speed();
		}
	}
	// We get here if the game is over.
	
	Tunes_Stop();
}

void handle_game_over() {
	move_terminal_cursor(10,14);
	printf_P(PSTR("GAME OVER"));
	move_terminal_cursor(10,15);
	printf_P(PSTR("Press a button or 's'/'S' to start a new game"));
	
	Tunes_Play_star();
	
	// Do nothing until a button is pushed. Hint: 's'/'S' should also start a
	// new game
	while (button_pushed() == NO_BUTTON_PUSHED && !start_input_pressed()) {
		char serial_input = get_serial_input();
		if((char)tolower(serial_input) == 'm') toggle_mute();
		if(Tunes_IsPlaying()) Tunes_Think(); // wait
	}
	
	Tunes_Stop();
}

void draw_game_speed(int8_t speed) {
	char game_speed[] = "Slow";
	
	switch(speed){
		case FAST_GAME_SPEED:
			strcpy(game_speed, "Fast");
			break;
		case MEDIUM_GAME_SPEED:
			strcpy(game_speed, "Medium");
			break;
		default:
			break;
	}
	
	move_terminal_cursor(10,5);
	// Clear the line so we don't get mashed words
	clear_to_end_of_line();
	printf_P(PSTR("Current Ball Speed: %s"), game_speed);
}

void handle_serial_input(char input){
	
	// Check inputs that can't be lowered first
	switch(input){
		case '1':
			set_game_speed(SLOW_GAME_SPEED);
			draw_game_speed(SLOW_GAME_SPEED);
			break;
		case '2':
			set_game_speed(MEDIUM_GAME_SPEED);
			draw_game_speed(MEDIUM_GAME_SPEED);
			break;
		case '3':
			set_game_speed(FAST_GAME_SPEED);
			draw_game_speed(FAST_GAME_SPEED);
			break;
		default:
			break;
	}
	
	switch((char)tolower(input)){
		case 'k':
			handle_keyboard_movement(INPUT_K_PRESSED);
			break;
		case 'l':
			handle_keyboard_movement(INPUT_L_PRESSED);
			break;
		case 'o':
			handle_keyboard_movement(INPUT_O_PRESSED);
			break;
		case 'd':
			handle_keyboard_movement(INPUT_D_PRESSED);
			break;
		case 's':
			handle_keyboard_movement(INPUT_S_PRESSED);
			break;
		case 'w':
			handle_keyboard_movement(INPUT_W_PRESSED);
			break;
		case 'p':
			if(toggle_pause()){
				remaining_time = next_ball_move_time - current_time;
			}else{
				next_ball_move_time = current_time + remaining_time;
			}
			break;
		case 'c':
			toggle_cpu_enabled();
			break;
		case 'm':
			toggle_mute();
			break;
		default:
			break;
	}
}

void handle_keyboard_movement(int8_t move){
	btn |= move;
}