#pragma once
#include <pico/time.h>

namespace protocol {
enum class State {
    Off,
    Standby,
    Brew,
};

void init();
void main_loop();
void set_power(bool on);
void network_loop();
State get_state();
absolute_time_t get_machine_start_time();
absolute_time_t get_state_change_time();
}
