#pragma once
#include "inttypes.hpp"

struct Settings {
    float brew_temp;
    float steam_temp;
    float brew_pressure;
    float preinfusion_pressure;
    float preinfusion_time;
    float brew_time;
    float kp;
    float ki;
    float kd;

    void write(u8*& ptr) const;
    void read(u8*& ptr);
};

namespace settings {
void init();
const Settings& get();
void update(Settings& new_settings);
}
