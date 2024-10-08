#include "control.hpp"
#include <pico/time.h>
#include "hardware/timer.h"
#include "impl/kalman_filter.hpp"
#include "impl/pid.hpp"
#include "hardware/hardware.hpp"
#include "pump.hpp"
#include "protocol.hpp"
using namespace control;

Sensors _sensors;

// Sensor update vars
static SimpleKalmanFilter pressure_filter(0.6, 0.6, 0.1);
static SimpleKalmanFilter temp_filter(0.5, 0.5, 0.3);
static absolute_time_t pressure_read_timeout = nil_time;
static absolute_time_t temp_read_timeout = nil_time;
static absolute_time_t close_enough_time = nil_time;
static bool last_close_enough = false;

// Blink vars
static u32 blink_light_period = 0;
static absolute_time_t blink_timeout = nil_time;
static bool blink_light_on = false;

// Update vars
static bool heater_enabled = false;
static bool pump_enabled = false;
static absolute_time_t heater_update_timeout = nil_time;
static float target_pressure;
static PID heater_pid(0.087, 0.00383, 0.49416, 0, 1);

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

void control::set_light_blink(u32 delay_ms) {
    hardware::set_light(hardware::Steam, false);
    blink_light_period = delay_ms;
    blink_light_on = false;
    blink_timeout = nil_time;
}

void control::reset() {
    pressure_filter.reset(hardware::read_pressure());
    temp_filter.reset(hardware::read_temp());
    heater_pid.reset(_sensors.temperature);
}

void control::update_sensors() {
    if (time_reached(pressure_read_timeout)) {
        float raw_pressure = hardware::read_pressure();
        _sensors.pressure = pressure_filter.update(raw_pressure);
        pressure_read_timeout = make_timeout_time_ms(10);
    }

    if (time_reached(temp_read_timeout)) {
        float raw_temp = hardware::read_temp();
        _sensors.temperature = temp_filter.update(raw_temp);
        temp_read_timeout = make_timeout_time_ms(250);
    }

    _sensors.weight = hardware::read_weight();
}

void control::update() {
    if (heater_enabled) {
        if (!time_reached(heater_update_timeout))
            return;
        heater_update_timeout = make_timeout_time_ms(250);

        float curr_temp = _sensors.temperature;

        float heater_value = heater_pid.update(curr_temp);
        hardware::set_heater(heater_value);

        bool temp_close_enough = fabs(heater_pid.get_target() - curr_temp) < 1;
        if (temp_close_enough != last_close_enough) {
            last_close_enough = temp_close_enough;
            close_enough_time = make_timeout_time_ms(30000);
        }

        auto& state = protocol::state();

        hardware::set_light(hardware::Brew, temp_close_enough && time_reached(close_enough_time));
    }

    if (pump_enabled) {
        float curr_pressure = _sensors.pressure;

        float pump_value = getPumpPct(curr_pressure, target_pressure);
        hardware::set_pump(pump_value);
    }

    if (blink_light_period > 0 && time_reached(blink_timeout)) {
        blink_light_on = !blink_light_on;
        hardware::set_light(hardware::Steam, blink_light_on);
        blink_timeout = make_timeout_time_ms(blink_light_period);
    }
}

const Sensors& control::sensors() {
    return _sensors;
}
