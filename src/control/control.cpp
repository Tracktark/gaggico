#include "control.hpp"
#include <pico/time.h>
#include "impl/kalman_filter.hpp"
#include "impl/pid.hpp"
#include "hardware/hardware.hpp"
#include "pump.hpp"
#include "protocol.hpp"
using namespace control;

Sensors _sensors;

static PID heater_pid(0.087, 0.00383, 0.49416, 0, 1);
static SimpleKalmanFilter pressure_filter(0.6, 0.6, 0.1);
static SimpleKalmanFilter temp_filter(0.5, 0.5, 0.3);

static bool heater_enabled = false;
static bool pump_enabled = false;
static float target_pressure = false;
static u32 blink_light_period = 0;
static absolute_time_t blink_timeout = nil_time;
static bool blink_light_on = false;

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
        auto& state = protocol::state();
        bool five_min_since_start =
            absolute_time_diff_us(state.machine_start_time, get_absolute_time()) > 7 * 60 * 1'000'000;
        hardware::set_light(hardware::Brew, temp_close_enough && (!state.cold_start || five_min_since_start));
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
