#pragma once
#include <pico/time.h>
#include <hardware/gpio.h>
#include "inttypes.hpp"

struct PSM {
    absolute_time_t next_update_time;

    i32 accumulator = 0;
    i32 max_value;
    i32 set_value = 0;

    volatile u32 counter = 0;
    u32 control_pin;

    u32 divider_counter = 0;

    explicit PSM(u32 control_pin, u32 max_value)
        : max_value(max_value), control_pin(control_pin) {

        gpio_init(control_pin);
        gpio_set_dir(control_pin, true);
        gpio_put(control_pin, false);
    }

    void zero_crossing_handler() {
        if (!time_reached(next_update_time))
            return;
        next_update_time = make_timeout_time_ms(6);

        if (divider_counter < 1) {
            divider_counter++;
            return;
        }
        divider_counter = 0;

        accumulator += set_value;
        if (accumulator >= max_value) {
            accumulator -= max_value;
            gpio_put(control_pin, true);
            counter += 1;
        } else {
            gpio_put(control_pin, false);
        }
    }

    constexpr void set(u32 value) {
        set_value = MIN(value, max_value);
        if (value == 0) {
            gpio_put(control_pin, false);
        }
    }
};
