
/*
 * adc.c
 *
 *  Created on: Apr 1, 2023
 *      Author: Doug
 */
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define ADC_PIN 28
#define ADC_CHANNEL 2

// Fixed point scaling: (10k + 3k) / 3k = 4.3333...
// We use a large multiplier (1024) to maintain precision in integer math
#define SCALE_FACTOR_NUM 13
#define SCALE_FACTOR_DEN 3

// --- CALIBRATION ---
// Adjusting for multimeter reading (7.501V) vs Pico display (7.553V)
#define CALIB_NUM 7501
#define CALIB_DEN 7553

void init_adc()
{
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);
}

/**
 * Returns battery voltage in Millivolts (e.g. 12600 for 12.6V)
 */
uint32_t get_battery_mv()
{
    // 1. Get raw 12-bit ADC value (0-4095)
    uint16_t raw = adc_read();

    // 2. Calculate voltage at the pin in millivolts
    // Pin MV = (raw * 3300) / 4095
    uint32_t pin_mv = (raw * 3300) / 4095;

    // 3. Scale up to actual battery voltage using the divider ratio (13/3)
    uint32_t battery_mv = (pin_mv * SCALE_FACTOR_NUM) / SCALE_FACTOR_DEN;

    // 3. Apply calibration factor based on multimeter reading
    return (battery_mv);
    // return (battery_mv * CALIB_NUM) / CALIB_DEN;
}

/**
 * A simple Integer Moving Average filter
 * alpha_num: smoothing strength (lower = smoother, e.g., 1 or 2)
 * alpha_den: scale (e.g., 10)
 */
uint32_t get_smoothed_mv()
{
    static uint64_t filtered_mv = 0;
    uint32_t current_mv = get_battery_mv();

    if (filtered_mv == 0)
    {
        filtered_mv = current_mv;
    }
    else
    {
        // Integer-based Low Pass Filter:
        // filtered = ((current * 2) + (filtered * 8)) / 10
        filtered_mv = (uint64_t)((current_mv * 2) + (filtered_mv * 8)) / 10;
    }
    return (uint32_t)filtered_mv;
}
