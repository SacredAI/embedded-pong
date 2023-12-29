/*
 * game.h
 *
 * Author: Jarrod Bennett, Cody Burnett
 *
 * Function prototypes for game functions available externally. You may wish
 * to add extra function prototypes here to make other functions available to
 * other files.
 */

#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>

// Ball directions
#define LEFT				(-1)
#define RIGHT				(1)
#define DOWN				(-1)
#define UP					(1)
#define STATIONARY			(0)

#define BALL_START_X_DIR	(RIGHT)
#define BALL_START_Y_DIR	(STATIONARY)

// Game board dimensions (x and y axis are as per LED matrix i.e. x is the
// longer axis)
#define BOARD_WIDTH			(12)
#define BOARD_HEIGHT		(8)
#define GAME_BORDER_WIDTH	(1)

#define PLAYER_1_X			(0)
#define PLAYER_2_X			(BOARD_WIDTH - 1)
#define PLAYER_HEIGHT		(2)

#define BALL_START_X		(BOARD_WIDTH / 2 - 1)
#define BALL_START_Y		(BOARD_HEIGHT / 2)

#define PLAYER_1			(0)
#define PLAYER_2			(1)

// Game objects
#define EMPTY_SQUARE		(0)
#define PLAYER				(1)
#define BALL				(2)
#define OBSTACLE			(3)
#define GUIDE				(4)

// Move 
#define NO_MOVEMENT			(0)
#define PLAYER_2_DOWN		(1)
#define PLAYER_2_UP			(2)
#define PLAYER_1_DOWN		(4)
#define PLAYER_1_UP			(8)

// Score
#define WIN_SCORE			(3)

// Game Speed
#define SLOW_GAME_SPEED		(0)
#define MEDIUM_GAME_SPEED	(1)
#define FAST_GAME_SPEED		(2)

struct ball_data {
	int8_t ball_x;
	int8_t ball_y;
	int8_t ball_x_direction;
	int8_t ball_y_direction;
};

// Initialise the player paddles, ball and display to start a game of PONG.
void initialise_game(void);

void update_guide_paddle(int8_t y);

// Try and move the selected player's y coordinate by the amount specified.
// For example, to move player 1's paddle up one space, call the function 
// as `move_player(PLAYER_1, 1)`. Use -1 to move the paddle down. No pixels of
// the player paddles should be allowed to move off the display.
void move_player_paddle(int8_t player, int8_t direction);

// Update ball position based on current x direction and y direction of ball
void update_ball_position(void);


// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void);

uint8_t get_winner(void);

// Returns 1 if game is paused, 0 otherwise
uint8_t is_game_paused(void);

// Returns 1 if puased game, 0 if started game
uint8_t toggle_pause(void);

// Translate terminal and/or button presses to movement
void handle_player_move(int8_t move);

void reset_ball(void);

void display_players_score(void);

int8_t get_player_score(int8_t player);

uint32_t get_game_speed(void);

void set_game_speed(uint8_t speed);

void get_ball_data(struct ball_data*);

int8_t get_player_y(int8_t player);

int8_t get_player_x(int8_t player);

int8_t get_guide_y(void);

#endif
