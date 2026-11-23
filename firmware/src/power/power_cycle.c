/**
 * @file power_cycle.c
 * @brief Target board power cycling implementation
 */

#include "power_cycle.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

void power_cycle_init(uint32_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 1); /* Active HIGH (powered) */
}

void power_cycle_execute(uint32_t pin, uint32_t off_duration_ms) {
    /* Pull LOW to cut power */
    gpio_put(pin, 0);
    sleep_ms(off_duration_ms);
    /* Restore power */
    gpio_put(pin, 1);
}
