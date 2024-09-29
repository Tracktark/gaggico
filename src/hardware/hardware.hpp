#pragma once

#include "inttypes.hpp"
namespace hardware {
enum Switch {
    Power, Brew, Steam,
};

void init();
void set_heater(float val);
void set_pump(float val);
u32 get_and_reset_pump_clicks();
void set_solenoid(bool active);
void set_light(Switch which, bool active);
bool get_switch(Switch which);
float read_pressure();
float read_temp();
float read_weight();
void scale_start_tare();
void scale_tare_immediately();
bool is_scale_connected();
bool is_scale_taring();
bool is_power_just_pressed();
}
