#pragma once
#include <pico/time.h>

namespace protocol {
};

void init();
void main_loop();
void set_power(bool on);
void network_loop();
absolute_time_t get_machine_start_time();
absolute_time_t get_state_change_time();
void on_state_change(int old_state_id, int new_state_id);
int get_state_id();
}
