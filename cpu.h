/*
 * cpu.h
 *
 * Created: 13/05/2023 10:37:40 PM
 *  Author: Alex Donnellan
 */ 

#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>
#include "game.h"

struct prediction {
	int8_t y;
	int8_t player_x;
};

// How many ms between each cpu move
#define CPU_MOVE_DELAY		100
#define CPU_PLAYER		PLAYER_1

uint8_t is_cpu_enabled(void);

void toggle_cpu_enabled(void);

void cpu_think(void);

void guide_think(void);

void predict_ball(struct prediction* p);

#endif /* CPU_H_ */