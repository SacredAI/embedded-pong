/*
 * game.c
 *
 * Functionality related to the game state and features.
 *
 * Author: Jarrod Bennett, Cody Burnett
 */ 

#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include "sound.h"

#include "timer0.h"
#include "display.h"
#include "terminalio.h"
#include "ssd.h"
#include "cpu.h"

// Player paddle positions. y coordinate refers to lower pixel on paddle.
// x coordinates never change but are nice to have here to use when drawing to
// the display.
static const int8_t PLAYER_X_COORDINATES[] = {PLAYER_1_X, PLAYER_2_X};
static const uint8_t score_start_x[] = {SCORE_1_START_X, SCORE_2_START_X};
static const uint8_t rally_start_x[] = {RALLY_1_X, RALLY_2_X};
static int8_t player_y_coordinates[] = {0, 0};
static int8_t player_score[] = {0, 0};
static int8_t player_rally[] = {-1, -1};
static int8_t guide_y_coordinate = 0;

// Ball position
int8_t ball_x;
int8_t ball_y;

// Ball direction
int8_t ball_x_direction;
int8_t ball_y_direction;

// Prevent point from being gained by pausing and un pausing game on goal
uint8_t gained_point = 0;

// 1 game is paused, 0 normal operation
uint8_t game_paused;

// Allow unpausing of the game after a set time
uint32_t resume_time = 0;

// Draw Prototypes
void draw_player_paddle(uint8_t player_to_draw);
void erase_player_paddle(uint8_t player_to_draw);

// Random number prototypes
int8_t generate_random_number(int8_t min_number, int8_t max_number);
int8_t random_x_direction(void);
int8_t random_y_direction(void);

// Collision Prototypes
uint8_t check_ball_collision(int8_t *new_ball_x, int8_t *new_ball_y);
uint8_t check_ball_collision_with_player(int8_t player, int8_t new_ball_x, int8_t new_ball_y);
uint8_t check_ball_collision_with_playerEX(int8_t player_x, int8_t player_y, int8_t new_ball_x, int8_t new_ball_y);
uint8_t check_ball_collision_with_players(int8_t new_ball_x, int8_t new_ball_y);
void check_vertical_ball_collision(int8_t *new_ball_x, int8_t *new_ball_y);
void draw_player_score(int8_t player);
void clear_player_score(int8_t player);

// Score prototypes
void add_point(int8_t player);
void reset_rally_counters(void);
void reset_rally_counter(int8_t player);
void increment_rally_counter(int8_t player);

void erase_guide_paddle(void);
void draw_guide_paddle(void);

// Initialise the player paddles, ball and display to start a game of PONG.
void initialise_game(void) {
	// initialise the display we are using.
	initialise_display();

	// Start players in the middle of the board
	player_y_coordinates[PLAYER_1] = BOARD_HEIGHT / 2 - 1;
	player_y_coordinates[PLAYER_2] = BOARD_HEIGHT / 2 - 1;

	player_score[PLAYER_1] = 0;
	player_score[PLAYER_2] = 0;
	
	display_players_score();
	ssd_display_score();

	draw_player_paddle(PLAYER_1);
	draw_player_paddle(PLAYER_2);
	
	clear_player_score(PLAYER_1);
	clear_player_score(PLAYER_2);
	
	reset_rally_counters();
	// Set Random Seed
	srand(get_current_time());
	
	reset_ball();
	
	// Draw new ball
	update_square_colour(ball_x, ball_y, BALL);
	
	// Set Pin D3 to be an output
	DDRD |= (1<<DDD3);
}

void reset_ball(void){
	// Clear the old ball
	update_square_colour(ball_x, ball_y, EMPTY_SQUARE);
	
	// Reset ball position and direction
	ball_x = BALL_START_X;
	ball_y = BALL_START_Y;

	ball_x_direction = random_x_direction();
	ball_y_direction = random_y_direction();
	
	gained_point = 0;
}

void update_guide_paddle(int8_t y){
	erase_guide_paddle();
	guide_y_coordinate = y;
	draw_guide_paddle();
}

void draw_guide_paddle(void){
	int8_t guide_x = PLAYER_X_COORDINATES[PLAYER_2];
	
	for (int y = guide_y_coordinate; y < guide_y_coordinate + PLAYER_HEIGHT; y++) {
		if(y == player_y_coordinates[PLAYER_2] || y == player_y_coordinates[PLAYER_2] + 1) continue;
		update_square_colour(guide_x, y, GUIDE);
	}
}

void erase_guide_paddle(void) {
	int8_t guide_x = PLAYER_X_COORDINATES[PLAYER_2];
	
	for (int y = guide_y_coordinate; y < guide_y_coordinate + PLAYER_HEIGHT; y++) {
		if(y == player_y_coordinates[PLAYER_2] || y == player_y_coordinates[PLAYER_2] + 1) continue;
		update_square_colour(guide_x, y, EMPTY_SQUARE);
	}
}

// Draw player 1 or 2 on the game board at their current position (specified
// by the `PLAYER_X_COORDINATES` and `player_y_coordinates` variables).
// This makes it easier to draw the multiple pixels of the players.
void draw_player_paddle(uint8_t player_to_draw) {
	int8_t player_x = PLAYER_X_COORDINATES[player_to_draw];
	int8_t player_y = player_y_coordinates[player_to_draw];

	for (int y = player_y; y < player_y + PLAYER_HEIGHT; y++) {
		update_square_colour(player_x, y, PLAYER);
	}
	
	if(is_cpu_enabled() && player_to_draw == PLAYER_2){
		erase_guide_paddle();
		draw_guide_paddle();
	}
}

// Erase the pixels of player 1 or 2 from the display.
void erase_player_paddle(uint8_t player_to_draw) {
	int8_t player_x = PLAYER_X_COORDINATES[player_to_draw];
	int8_t player_y = player_y_coordinates[player_to_draw];

	for (int y = player_y; y < player_y + PLAYER_HEIGHT; y++) {
		update_square_colour(player_x, y, EMPTY_SQUARE);
	}
}

// Try and move the selected player's y coordinate by the amount specified.
// For example, to move player 1's paddle up one space, call the function
// as `move_player(PLAYER_1, 1)`. Use `-1` instead to move the paddle down. No
// pixels of the player paddles should be allowed to move off the display.
void move_player_paddle(int8_t player, int8_t direction) {
	/* suggestions for implementation:
	 * 1: Figure out the new location of the player paddle. Consider what should
	 *	  happen if the player paddle is at the edge of the board.
	 * 2: Remove the player from the display at the current position.
	 * 3: Update the positional knowledge of the player. This will involve the
	 *    player coordinate variables.
	 * 4: Display the player at their new position.
	 */	
	int8_t player_y = player_y_coordinates[player];
	
	player_y += direction;
	
	if((player_y + PLAYER_HEIGHT) > BOARD_HEIGHT){
		return;
	}else if (player_y < 0) return;
	else if (check_ball_collision_with_playerEX(PLAYER_X_COORDINATES[player], player_y, ball_x, ball_y)) return;
	
	erase_player_paddle(player);
	
	player_y_coordinates[player] = player_y;
	
	draw_player_paddle(player); 
}

// Update ball position based on current x direction and y direction of ball
void update_ball_position(void) {
	

	// Determine new ball coordinates
	int8_t new_ball_x = ball_x + ball_x_direction;
	int8_t new_ball_y = ball_y + ball_y_direction;
	
	if(check_ball_collision(&new_ball_x, &new_ball_y)){
		return;
	}
	
	// Erase old ball
	update_square_colour(ball_x, ball_y, EMPTY_SQUARE);
	
	// Assign new ball coordinates
	ball_x = new_ball_x;
	ball_y = new_ball_y;
	
	// Draw new ball
	update_square_colour(ball_x, ball_y, BALL);
}

// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void) {
	if(resume_time){
		uint32_t current_time = get_current_time();
		if(current_time >= resume_time){
			resume_time = 0;
			reset_ball();
			toggle_pause();
			clear_player_score(PLAYER_1);
			clear_player_score(PLAYER_2);
		}
	}
	return (player_score[PLAYER_1] == WIN_SCORE || player_score[PLAYER_2] == WIN_SCORE);
}

uint8_t get_winner(void){
	if(!is_game_over()) return 0;
	return (player_score[PLAYER_1] == WIN_SCORE)?1:2;
}

uint8_t is_game_paused(void){
	return game_paused;
}

uint8_t toggle_pause(void){
	game_paused ^= 1;
	if(game_paused){
		move_terminal_cursor(10,8);
		printf_P(PSTR("GAME PAUSED!"));
		PORTD |= (1<<PORTD3);
	}else{
		move_terminal_cursor(10,8);
		clear_to_end_of_line();
		PORTD = (PORTD & ~(1<<PORTD3));
	}
	return game_paused;
}

// Translate terminal and/or button presses to movement
void handle_player_move(int8_t move) {
	if(is_game_paused()) return;
	if((move & PLAYER_2_DOWN) && !( move & PLAYER_2_UP)){
		move_player_paddle(PLAYER_2, DOWN);
	}
	if((move & PLAYER_2_UP) && !( move & PLAYER_2_DOWN)){
		move_player_paddle(PLAYER_2, UP);
	}
	if(is_cpu_enabled()) return;
	if((move & PLAYER_1_DOWN) && !(move & PLAYER_1_UP)){
		move_player_paddle(PLAYER_1, DOWN);
	}
	if((move & PLAYER_1_UP) && !(move & PLAYER_1_DOWN)){
		move_player_paddle(PLAYER_1, UP);
	}
}

uint8_t check_ball_collision(int8_t *new_ball_x, int8_t *new_ball_y) {
	
	check_vertical_ball_collision(new_ball_x, new_ball_y);

	if(check_ball_collision_with_players(*new_ball_x, *new_ball_y)){
		ball_y_direction = random_y_direction();
		ball_x_direction *= -1;
		*new_ball_x = ball_x + ball_x_direction;
		*new_ball_y = ball_y + ball_y_direction;
		
		Tone(NOTE_C7, 100);
		
		// Ensure we don't go off the board
		check_vertical_ball_collision(new_ball_x, new_ball_y);
	}
	
	if(*new_ball_x > (BOARD_WIDTH - GAME_BORDER_WIDTH)){
		add_point(PLAYER_1);
		//reset_ball();
		return 1;
	}
	
	if(*new_ball_x < 0){
		add_point(PLAYER_2);
		//reset_ball();
		return 1;
	}
	
	return 0;
}

void check_vertical_ball_collision(int8_t *new_ball_x, int8_t *new_ball_y){
	if(*new_ball_y >= BOARD_HEIGHT || *new_ball_y < 0){
		ball_y_direction *= -1;
		*new_ball_y = ball_y + ball_y_direction;
	}
}

uint8_t check_ball_collision_with_players(int8_t new_ball_x, int8_t new_ball_y){
	if(check_ball_collision_with_player(PLAYER_1, new_ball_x, new_ball_y)){
		increment_rally_counter(PLAYER_1);
		return 1;
	}else if (check_ball_collision_with_player(PLAYER_2, new_ball_x, new_ball_y)){
		increment_rally_counter(PLAYER_2);
		return 1;
	}
	return 0;
}

uint8_t check_ball_collision_with_player(int8_t player, int8_t new_ball_x, int8_t new_ball_y){
	int8_t player_x = PLAYER_X_COORDINATES[player];
	int8_t player_y = player_y_coordinates[player];
	return check_ball_collision_with_playerEX(player_x, player_y, new_ball_x, new_ball_y);
}

uint8_t check_ball_collision_with_playerEX(int8_t player_x, int8_t player_y, int8_t new_ball_x, int8_t new_ball_y){
	// If x doesn't match we're colliding
	if(player_x != new_ball_x) return 0;
	
	for (int y = player_y; y < player_y + PLAYER_HEIGHT; y++) {
		if(y == new_ball_y) return 1;
	}
	return 0;
}

int8_t generate_random_number(int8_t min_number, int8_t max_number) {
	return (rand() % (max_number + 1 - min_number)) + min_number;
}

int8_t random_x_direction(void){
	return (generate_random_number(0, 1))?1:-1;
}

int8_t random_y_direction(void){
	return generate_random_number(-1, 1);
}

void display_players_score(void){
	move_terminal_cursor(10,10);
	printf_P(PSTR("Player 1 Score: %d"), player_score[PLAYER_1]);
	move_terminal_cursor(10,12);
	printf_P(PSTR("Player 2 Score: %d"), player_score[PLAYER_2]);
}

void add_point(int8_t player){
	if(gained_point) return;
	player_score[player] += 1;
	
	gained_point = 1;
	
	reset_rally_counters();
	
	display_players_score();
	draw_player_score(PLAYER_1);
	draw_player_score(PLAYER_2);
	if(!is_game_over()){
		toggle_pause();
		// Wait 1500ms before resuming
		uint32_t current_time = get_current_time();
		resume_time = current_time + 1500;
	}
}

int8_t get_player_score(int8_t player){
	return player_score[player];
}

void draw_player_score(int8_t player){
	draw_3x3_number(score_start_x[player], SCORE_START_Y, get_player_score(player));
}

void clear_player_score(int8_t player){
	clear_3x3_grid(score_start_x[player], SCORE_START_Y);
}

void reset_rally_counters(void){
	reset_rally_counter(PLAYER_1);
	reset_rally_counter(PLAYER_2);
}

void reset_rally_counter(int8_t player){
	player_rally[player] = -1;
	clear_rally_col(rally_start_x[player]);
}

void increment_rally_counter(int8_t player) {
	player_rally[player] += 1;
	// Fast Modulus 8
	uint8_t num = (player_rally[player] & ( 8 - 1)) + 1;
	draw_rally_count(rally_start_x[player], num);
}

static uint8_t game_speed = SLOW_GAME_SPEED; // Current Game Speed
static const uint32_t game_speeds[] = {500, 300, 200}; // Possible Game Speeds

uint32_t get_game_speed(void){
	return game_speeds[game_speed];
}

void set_game_speed(uint8_t speed){
	game_speed = speed;
}

void get_ball_data(struct ball_data* bd){
	bd->ball_x = ball_x;
	bd->ball_y = ball_y;
	bd->ball_x_direction = ball_x_direction;
	bd->ball_y_direction = ball_y_direction;
}

int8_t get_player_y(int8_t player){
	return player_y_coordinates[player];
}

int8_t get_player_x(int8_t player){
	return PLAYER_X_COORDINATES[player];
}

int8_t get_guide_y(){
	return guide_y_coordinate;
}