#pragma once
#include "inttypes.hpp"

struct Settings {
    float brew_temp = 97;
    float steam_temp = 150;
    float brew_pressure = 8;
    float preinfusion_pressure = 2;
    float preinfusion_time = 0;
    float brew_weight = -1;
    float kp = 0.087;
    float ki = 0.00383;
    float kd = 0.49416;

    void write_data(u8*& ptr) const;
    void read_data(u8*& ptr);
};

namespace settings {
void init();
const Settings& get();
void update(Settings& new_settings);
}
