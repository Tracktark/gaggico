#pragma once
#include <pico/time.h>

namespace protocol {
struct MachineState {
    absolute_time_t machine_start_time;
    absolute_time_t state_change_time;
    absolute_time_t brew_start_time;
    bool cold_start;
};

void init();
void main_loop();
void set_power(bool on);
void network_loop();
void on_state_change(int old_state_id, int new_state_id);
int get_state_id();
MachineState& state();
}
