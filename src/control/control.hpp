#pragma once

#include "inttypes.hpp"
namespace control {
struct Sensors {
    float pressure;
    float temperature;
    float weight;
};

void set_boiler_enabled(bool enabled);
void set_pump_enabled(bool enabled);
void set_target_pressure(float pressure);
void set_target_temperature(float temperature);
void set_pid_params(float kp, float kd, float ki);
void set_light_blink(u32 delay_ms);
void reset();
void update();
void update_sensors();
const Sensors& sensors();
}
