#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include "network/discovery.hpp"
#include "network/ntp.hpp"
#include "network/network.hpp"
#include "control/protocol.hpp"
#include "hardware/hardware.hpp"

static void core1_entry() {
    multicore_lockout_victim_init();
    network::wifi_init();
    discovery::init();
    ntp::init();
    network::server_init();

    protocol::network_loop();
}

int main() {
    stdio_init_all();
    multicore_lockout_victim_init();
    settings::init();
    hardware::init();

    multicore_launch_core1(core1_entry);

    watchdog_enable(1000, true);

    while (true) {
        protocol::main_loop();
    }

    return 0;
}
