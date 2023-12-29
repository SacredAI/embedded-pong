/*
 * display.c
 *
 * Authors: Luke Kamols, Jarrod Bennett, Martin Ploschner, Cody Burnett,
 * Renee Nightingale
 */ 

#include "display.h"
#include <stdio.h>
#include "pixel_colour.h"
#include "ledmatrix.h"
#include "game.h"

// constant value used to display 'PONG' on launch
static const uint8_t pong_display[MATRIX_NUM_COLUMNS] = 
		{126, 72, 120, 127, 67, 127, 126, 64, 126, 127, 67, 79, 0, 2, 82, 64};

// Fonts for LED Matrix score display
// Stored as a 5 x 3 grid pattern going from Left-to-Right, Top-to-Bottom
// Padded with a leading zero so that it fits into a 16-bit value
static const uint16_t LED_DIGIT_FONTS[10] = {
    0b0111101101101111, // 0
    0b0010010010010010, // 1
    0b0111100111001111, // 2
    0b0111001111001111, // 3
    0b0001001111101101, // 4
    0b0111001111100111, // 5
    0b0111101111100111, // 6
    0b0001001001001111, // 7
    0b0111101111101111, // 8
    0b0111001111101111, // 9
};

// Initialise the display for the board, this creates the display
// for an empty board.
void initialise_display(void) {
	// start by clearing the LED matrix
	ledmatrix_clear();

	// create an array with the background colour at every position
	PixelColour col_colours[MATRIX_NUM_ROWS];
	for (int row = 0; row < MATRIX_NUM_ROWS; row++) {
		col_colours[row] = MATRIX_COLOUR_BORDER;
	}

	// then add the bounds on the left
	for (int x = 1; x < 1 + GAME_BORDER_WIDTH; x++) {
		ledmatrix_update_column(x, col_colours);
	}

	// and add the bounds on the right
	for (int x = 14; x < 14 + GAME_BORDER_WIDTH; x++) {
		ledmatrix_update_column(x, col_colours);
	}
}

void show_start_screen(void) {
	PixelColour colour;
	MatrixColumn column_colour_data;
	uint8_t col_data;
		
	ledmatrix_clear(); // start by clearing the LED matrix
	for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++) {
		col_data = pong_display[col];
		// using the LSB as the colour determining bit, 1 is red, 0 is green
		if (col_data & 0x01) {
			colour = COLOUR_RED;
		} else {
			colour = COLOUR_GREEN;
		}
		// go through the top 7 bits (not the bottom one as that was our colour bit)
		// and set any to be the correct colour
		for(uint8_t i = 7; i >= 1; i--) {
			// If the relevant font bit is set, we make this a coloured pixel, else blank
			if(col_data & 0x80) {
				column_colour_data[i] = colour;
			} else {
				column_colour_data[i] = 0;
			}
			col_data <<= 1;
		}
		column_colour_data[0] = 0;
		ledmatrix_update_column(col, column_colour_data);
	}
		// Update pong ball colour
	ledmatrix_update_pixel(START_SCREEN_BALL_X, START_SCREEN_BALL_Y, MATRIX_COLOUR_BALL);
}

// Update dynamic start screen based on the frame number (0-11)
// Note: this is hardcoded to PONG game.
// Purposefully obfuscated so functionality cannot be copied for movement tasks
void update_start_screen(uint8_t frame_number) {
	
	if (frame_number < 0 || frame_number > 11) {
		return;
	}
	
	// Clear the dynamic columns
	MatrixColumn column_colour_data[PONG_NUM_DYNAMIC_COLS];
	for (uint8_t col = 0; col < PONG_NUM_DYNAMIC_COLS; col++) {
		for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++) {
			column_colour_data[col][row] = 0;
		}
	}
	
	// Set the middle paddle pixel
	column_colour_data[1][6] = MATRIX_COLOUR_PLAYER;
	column_colour_data[1][1] = MATRIX_COLOUR_PLAYER;
	
	// Set the edge paddle pixel depending on frame number
	if (frame_number < 3 || frame_number >= 9) {
		column_colour_data[2][6] = MATRIX_COLOUR_PLAYER;
	} else {
		column_colour_data[0][6] = MATRIX_COLOUR_PLAYER;
	}
	if (frame_number < 6) {
		column_colour_data[0][1] = MATRIX_COLOUR_PLAYER;
	} else {
		column_colour_data[2][1] = MATRIX_COLOUR_PLAYER;
	}
	
	// Set the ball pixel depending on frame number
	if (frame_number == 5 || frame_number == 11) {
		column_colour_data[1][5] = MATRIX_COLOUR_BALL;
	} else if (frame_number == 0 || frame_number == 4 || frame_number == 6
			|| frame_number == 10) {
		column_colour_data[1][4] = MATRIX_COLOUR_BALL;
	} else if (frame_number == 1 || frame_number == 3 || frame_number == 7
			|| frame_number == 9) {
		column_colour_data[1][3] = MATRIX_COLOUR_BALL;
	} else {
		column_colour_data[1][2] = MATRIX_COLOUR_BALL;
	}

	// Update columns
	for (uint8_t col = 0; col < PONG_NUM_DYNAMIC_COLS; col++) {
		ledmatrix_update_column(col + PONG_DYNAMIC_COL_START, column_colour_data[col]);
	}
}

// Update the colour of the pixel at position (x, y) on the display to show the
// provided object
void update_square_colour(uint8_t x, uint8_t y, uint8_t object) {
	// determine which colour corresponds to this object
	PixelColour colour;
	
	switch (object) {
		case EMPTY_SQUARE:
			colour = MATRIX_COLOUR_EMPTY;
			break;
		case PLAYER:
			colour = MATRIX_COLOUR_PLAYER;
			break;
		case BALL:
			colour = MATRIX_COLOUR_BALL;
			break;
		case GUIDE:
			colour = MATRIX_COLOR_GUIDE;
			break;
		// An invalid/unexpected object
		default:
			colour = MATRIX_COLOUR_EMPTY;
			break;
	}

	// Update the pixel at the given location with this colour
	ledmatrix_update_pixel(x + MATRIX_X_OFFSET, y + MATRIX_Y_OFFSET, colour);
}

// Draw a three by three number from left to right bottom to top
void draw_3x3_number(uint8_t start_x, uint8_t start_y, uint8_t num){
	uint16_t digit = LED_DIGIT_FONTS[num];
	uint8_t offset_x =  0, offset_y = 0;
	for(uint8_t bit = 0; bit < 16; bit++){
		// Check if bit is turned on
		if(digit & (1<<bit)){
			ledmatrix_update_pixel(start_x - offset_x, start_y - offset_y, MATRIX_COLOR_SCORE);
		}
		if(offset_x == 2){
			offset_y++;
		}
		offset_x = (offset_x + 1) % 3;
	}
}

void clear_3x3_grid(uint8_t start_x, uint8_t start_y){
	uint8_t offset_x = 0, offset_y = 0;
	for(uint8_t bit = 0; bit < 16; bit++){
		// Check if bit is turned on
		ledmatrix_update_pixel(start_x - offset_x, start_y - offset_y, MATRIX_COLOUR_EMPTY);
		if(offset_x == 2){
			offset_y++;
		}
		offset_x = (offset_x + 1) % 3;
	}
}

void draw_rally_count(uint8_t x, uint8_t num) {
	MatrixColumn cols = { 0 };
	for(uint8_t y = 0; y < num; y++){
		cols[y] = MATRIX_COLOR_RALLY;
	}
	ledmatrix_update_column(x, cols);
}

void clear_rally_col(uint8_t x){
	MatrixColumn cols = { 0 };
	ledmatrix_update_column(x, cols);
}