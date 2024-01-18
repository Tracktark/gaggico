#include <pico/stdlib.h>
#include <pico/multicore.h>
#include "discovery.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "hardware.hpp"

static void core1_entry() {
    multicore_lockout_victim_init();
    network::wifi_init();
    discovery::init();
    network::server_init();

    protocol::network_loop();
}

int main() {
    stdio_init_all();
    multicore_lockout_victim_init();
    settings::init();
    hardware::init();

    multicore_launch_core1(core1_entry);

    while (true) {
        protocol::main_loop();
    }

    return 0;
}
