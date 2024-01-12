#include <pico/stdlib.h>
#include "discovery.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "hardware.hpp"

int main() {
    stdio_init_all();

    hardware::init();

    network::wifi_init();
    discovery::init();
    network::server_init();

    while (true) {
        protocol::main_loop();
    }

    return 0;
}
