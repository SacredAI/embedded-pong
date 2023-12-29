/*
 * cpu.c
 *
 * Created: 13/05/2023 10:37:29 PM
 *  Author: Alex Donnellan
 */ 

#include "cpu.h"
#include "game.h"
#include "timer0.h"

#include <limits.h>
#include <stdint.h>

// Things we know
// 1. when hitting upper/lower wall ball y direction inverts
// 2. CPU can move every 100 ms
// 3. We know ball direction
// 4. Need to extrapolate from ball direction (and any possible bounces) for optiomal position
// 5. Easy algorithm = match ball y axis

// 0 when human controlled, 1 for cpu player
static uint8_t cpu_enabled = 0;
static uint32_t next_move_time = 0, next_guide_move = 0, current_time = 0, last_predict_time;
static struct ball_data last_ball_data;
static struct prediction last_p;

static int8_t cpu_y_coordinate = 0;

uint8_t fastabs(int8_t v);

uint8_t is_cpu_enabled(void){
	return cpu_enabled;
}

void toggle_cpu_enabled(void){
	cpu_enabled ^= 1;
	if(cpu_enabled){
		next_move_time = current_time + 100;
	}
}

void cpu_think(void){
	current_time = get_current_time();
	cpu_y_coordinate = get_player_y(CPU_PLAYER);
	if(!is_cpu_enabled()) return;
	
	struct prediction p;
	p.player_x = get_player_x(CPU_PLAYER);
	predict_ball(&p);
	
	if(current_time >= next_move_time){
		if(p.y >= 0){
			if(p.y < cpu_y_coordinate){
				// move Down
				move_player_paddle(CPU_PLAYER, DOWN);
			}else if (p.y > cpu_y_coordinate){
				// move UP
				move_player_paddle(CPU_PLAYER, UP);
			}
		}
		
		next_move_time = current_time + 200;
	}
}

void guide_think(void){
	current_time = get_current_time();
	if(!is_cpu_enabled()) return;
	int8_t guide_y_coordinate = get_guide_y();

	struct prediction p;
	p.player_x = get_player_x(PLAYER_2);
	predict_ball(&p);
	
	if(current_time >= next_guide_move){
		if(p.y >= 0){
			if(p.y < guide_y_coordinate){
				// move Down
				update_guide_paddle(guide_y_coordinate + DOWN);
			}else if (p.y > guide_y_coordinate){
			// move UP
				update_guide_paddle(guide_y_coordinate + UP);
			}
		}
		next_guide_move = current_time + 200;
	}
}

void predict_ball(struct prediction* p){
	struct ball_data bd;
	get_ball_data(&bd);
	if((last_ball_data.ball_x_direction == bd.ball_x_direction) &&
		(last_ball_data.ball_y_direction == bd.ball_y_direction) &&
		(current_time < last_predict_time + 500)){
		//Nothing has changed and it hasn't been long since our last prediction so skip;
		
		p->y = last_p.y;
		return;		
	}
	if(fastabs(bd.ball_x - p->player_x) < fastabs((bd.ball_x + bd.ball_x_direction) - p->player_x)){
		// Ball moving away take a nap
		p->y = -1;
		return;
	}
	
	int8_t last_predict_x = bd.ball_x_direction;
	int8_t last_predict_y = bd.ball_y_direction;
	int8_t last_ball_x = bd.ball_x;
	int8_t last_ball_y = bd.ball_y;
	
	// Iterate through all remaining steps to the player
	while(last_ball_x != p->player_x){
		int8_t new_ball_x = last_ball_x + last_predict_x;
		int8_t new_ball_y = last_ball_y + last_predict_y;
		
		if(new_ball_y >= BOARD_HEIGHT || new_ball_y < 0){
			last_predict_y *= -1;
			new_ball_y = last_ball_y + last_predict_y;
		}
		
		last_ball_y = new_ball_y;
		last_ball_x = new_ball_x;
	}
	
	p->y = last_ball_y;
	
	last_ball_data = bd;
	last_p = *p;
}

uint8_t fastabs(int8_t v){
	int8_t const mask = (v >> sizeof(int8_t)) * CHAR_BIT - 1;
	return (v + mask) ^ mask;
}