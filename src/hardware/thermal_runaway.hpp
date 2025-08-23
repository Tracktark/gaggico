#pragma once

#include "inttypes.hpp"
#include <cstdio>
#include <pico/time.h>

// Assume enabled heater can raise the temperature of water by at least HEATING_GAIN °C over DEADLINE_S seconds
constexpr int HEATING_GAIN = 1;
constexpr int DEADLINE_S = 10;

struct ThermalRunawayCheck {
    absolute_time_t deadline = nil_time;
    float goal_temp = 0;

    bool has_fault(i32 heater_power, i32 pump_power, float temp) {
        bool deadline_enabled = heater_power > 90 && pump_power <= 1;
        if (!deadline_enabled) {
            deadline = nil_time;
            goal_temp = 0;
            return false;
        }

        if (time_reached(deadline) || temp >= goal_temp) {
            if (temp < goal_temp) {
                printf("Thermal runaway check failed, temp: %.2f, goal: %.2f, heater: %u%%, pump: %u%%\n",
                       temp, goal_temp, heater_power, pump_power);
                return true;
            }
            goal_temp = temp + HEATING_GAIN;
            deadline = make_timeout_time_ms(DEADLINE_S * 1000);
        }

        return false;
    }
};
