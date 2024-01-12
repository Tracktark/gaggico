#pragma once

namespace hardware {
enum Switch {
    Power, Brew, Steam, 
};

void init();
void set_heater(float val);
void set_pump(float val);
void set_solenoid(bool active);
void set_light(Switch which, bool active);
bool get_switch(Switch which);
float read_pressure();
float read_temp();
bool is_power_just_pressed();
}
