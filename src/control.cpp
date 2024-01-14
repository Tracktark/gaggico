#include "control.hpp"
#include <pico/time.h>
#include "hardware.hpp"
#include "pid.hpp"
using namespace control;


PID heater_pid(0.087, 0.00383, 0.49416, 0, 1);

bool heater_enabled = false;
void control::set_boiler_enabled(bool enabled) {
    heater_enabled = enabled;
    if (enabled) {
        heater_pid.reset(hardware::read_temp());
    } else {
        hardware::set_heater(0);
    }
}

void control::set_pump_enabled(bool enabled) {
    hardware::set_pump(enabled);
}

float target_pressure = false;
void control::set_target_pressure(float pressure) {
    target_pressure = pressure;
}

void control::set_target_temperature(float temperature) {
    heater_pid.set_target(temperature);
}

void control::update() {
    static absolute_time_t next_update_time = nil_time;
    if (!time_reached(next_update_time))
        return;
    next_update_time = make_timeout_time_ms(250);

    float curr_pressure = hardware::read_pressure();

    if (heater_enabled) {
        float curr_temp = hardware::read_temp();

        float heater_value = heater_pid.update(curr_temp);
        hardware::set_heater(heater_value);

        bool temp_close_enough = fabs(heater_pid.get_target() - curr_temp) < 1;
        hardware::set_light(hardware::Steam, temp_close_enough);
    }
}
