#include "control.hpp"
#include <pico/time.h>
#include "hardware/hardware.hpp"
#include "impl/pid.hpp"
#include "pump.hpp"
using namespace control;


PID heater_pid(0.087, 0.00383, 0.49416, 0, 1);
bool heater_enabled = false;

bool pump_enabled = false;
float target_pressure = false;

void control::set_boiler_enabled(bool enabled) {
    heater_enabled = enabled;
    if (enabled) {
        heater_pid.reset(hardware::read_temp());
    } else {
        hardware::set_heater(0);
    }
}

void control::set_pump_enabled(bool enabled) {
    pump_enabled = enabled;
    if (!enabled) {
        hardware::set_pump(0);
    }
}

void control::set_target_pressure(float pressure) {
    target_pressure = pressure;
}

void control::set_target_temperature(float temperature) {
    heater_pid.set_target(temperature);
}

void control::update() {
    if (heater_enabled) {
        static absolute_time_t heater_update_timeout = nil_time;
        if (!time_reached(heater_update_timeout))
            return;
        heater_update_timeout = make_timeout_time_ms(250);

        float curr_temp = hardware::read_temp();

        float heater_value = heater_pid.update(curr_temp);
        hardware::set_heater(heater_value);

        bool temp_close_enough = fabs(heater_pid.get_target() - curr_temp) < 1;
        hardware::set_light(hardware::Brew, temp_close_enough);
    }


    if (pump_enabled) {
        float curr_pressure = hardware::read_pressure();

        float pump_value = getPumpPct(curr_pressure, target_pressure);
        hardware::set_pump(pump_value);
    }
}
