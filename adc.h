
/*
 * adc.h
 *
 *  Created on: Apr 1, 2023
 *      Author: Doug
 */

#ifndef ADC_H
#define ADC_H

#include "hardware/adc.h"
#include <stdint.h>

void init_adc();
uint32_t get_battery_mv();
uint32_t get_smoothed_mv();

#endif  // ADC_H
