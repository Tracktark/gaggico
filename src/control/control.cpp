#include "control.hpp"
#include <pico/time.h>
#include "impl/kalman_filter.hpp"
#include "impl/pid.hpp"
#include "hardware/hardware.hpp"
#include "pump.hpp"
using namespace control;

Sensors _sensors;

static PID heater_pid(0.087, 0.00383, 0.49416, 0, 1);
static SimpleKalmanFilter pressure_filter(0.6, 0.6, 0.1);
static SimpleKalmanFilter temp_filter(0.5, 0.5, 0.01);
bool heater_enabled = false;

bool pump_enabled = false;
float target_pressure = false;

void control::set_boiler_enabled(bool enabled) {
    heater_enabled = enabled;
    if (enabled) {
        heater_pid.reset(_sensors.temperature);
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

void control::set_pid_params(float kp, float ki, float kd) {
    heater_pid.set_params(kp, ki, kd);
}

void control::reset() {
    pressure_filter.reset(hardware::read_pressure());
    temp_filter.reset(hardware::read_temp());
    heater_pid.reset(_sensors.temperature);
}

void control::update_sensors() {
    static absolute_time_t pressure_timeout = nil_time;
    if (time_reached(pressure_timeout)) {
        float raw_pressure = hardware::read_pressure();
        _sensors.pressure = pressure_filter.update(raw_pressure);
        pressure_timeout = make_timeout_time_ms(10);
    }

    static absolute_time_t temp_timeout = nil_time;
    if (time_reached(temp_timeout)) {
        float raw_temp = hardware::read_temp();
        _sensors.temperature = temp_filter.update(raw_temp);
        temp_timeout = make_timeout_time_ms(250);
    }

}

void control::update() {
    if (heater_enabled) {
        static absolute_time_t heater_update_timeout = nil_time;
        if (!time_reached(heater_update_timeout))
            return;
        heater_update_timeout = make_timeout_time_ms(250);

        float curr_temp = _sensors.temperature;

        float heater_value = heater_pid.update(curr_temp);
        hardware::set_heater(heater_value);

        bool temp_close_enough = fabs(heater_pid.get_target() - curr_temp) < 1;
        hardware::set_light(hardware::Brew, temp_close_enough);
    }


    if (pump_enabled) {
        float curr_pressure = _sensors.pressure;

        float pump_value = getPumpPct(curr_pressure, target_pressure);
        hardware::set_pump(pump_value);
    }
}

const Sensors& control::sensors() {
    return _sensors;
}
