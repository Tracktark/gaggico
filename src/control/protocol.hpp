#pragma once
#include <pico/time.h>

namespace protocol {
struct Times {
    absolute_time_t machine_start;
    absolute_time_t state_change;
    absolute_time_t brew_start;
};

void init();
void main_loop();
void set_power(bool on);
void network_loop();
void on_state_change(int old_state_id, int new_state_id);
int get_state_id();
Times& times();
}
