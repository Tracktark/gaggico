#pragma once

namespace control {

void set_boiler_enabled(bool enabled);
void set_pump_enabled(bool enabled);
void set_target_pressure(float pressure);
void set_target_temperature(float temperature);
void update();
}
