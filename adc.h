/*
 * adc.h
 *
 * Created: 14/05/2023 6:50:18 PM
 *  Author: Alex Donnellan
 */ 


#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

void init_adc_interrupts(void);

int8_t adc_move(void);

#endif /* ADC_H_ */