#include "control.hpp"
#include "hardware.hpp"
#include "messages.hpp"
#include <cstdio>
using namespace control;

bool boiler_enabled = false;
void control::set_boiler_enabled(bool enabled) {
    boiler_enabled = enabled;
    if (!enabled) {
        hardware::set_heater(0);
    }
}

bool pump_enabled = false;
void control::set_pump_enabled(bool enabled) {
    // pump_enabled = enabled;
    // if (!enabled) {
    //     hardware::set_pump(0);
    // }
    hardware::set_pump(enabled);
}

float target_pressure = false;
void control::set_target_pressure(float pressure) {
    target_pressure = pressure;
}

float target_temp = 0;
void control::set_target_temperature(float temperature) {
    target_temp = temperature;
}

void control::update() {
    float curr_temp = hardware::read_temp();
    float curr_pressure = hardware::read_pressure();
    // printf("Temp: %f\nPres: %f\n", curr_temp, curr_pressure);

    if (boiler_enabled) {
        if (curr_temp < target_temp) {
            hardware::set_light(hardware::Steam, true);
            hardware::set_heater(1);
        } else {
            hardware::set_light(hardware::Steam, false);
            hardware::set_heater(0);
        }
    }

    // if (pump_enabled) {
    //     if (curr_pressure < target_pressure) {
    //         hardware::set_pump(1);
    //     } else {
    //         hardware::set_pump(0);
    //     }
    // }
}
