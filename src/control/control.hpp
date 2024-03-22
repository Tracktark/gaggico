#pragma once

namespace control {
struct Sensors {
    float pressure;
    float temperature;
};

void set_boiler_enabled(bool enabled);
void set_pump_enabled(bool enabled);
void set_target_pressure(float pressure);
void set_target_temperature(float temperature);
void reset();
void update();
void update_sensors();
const Sensors& sensors();
}
