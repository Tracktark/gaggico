#pragma once

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
}
