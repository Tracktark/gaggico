#include "panic.hpp"
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include "hardware/hardware.hpp"
#include "hardware/watchdog.h"
#include "inttypes.hpp"
#include "pico/time.h"

constexpr auto DELAY_SHORT = 500;
constexpr auto DELAY_LONG = DELAY_SHORT * 5;

[[noreturn]]
void panic(Error error) {
    if (multicore_lockout_victim_is_initialized(1 - get_core_num())) {
        multicore_lockout_start_blocking();
    }

    u32 total_blinks = static_cast<u32>(error) + 1;
    u32 blinks = 0;
    bool light = true;
    absolute_time_t blink_timeout = nil_time;
    while (true) {
        hardware::set_heater(0);
        hardware::set_pump(0);
        hardware::set_solenoid(false);
        watchdog_update();

        if (time_reached(blink_timeout)) {
            hardware::set_light(hardware::Power, light);
            hardware::set_light(hardware::Brew, light);
            hardware::set_light(hardware::Steam, light);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, light);
            if (light) {
                blink_timeout = make_timeout_time_ms(DELAY_SHORT);
            } else {
                blink_timeout = make_timeout_time_ms(blinks == total_blinks - 1 ? DELAY_LONG : DELAY_SHORT);
                blinks = (blinks + 1) % total_blinks;
            }
            light = !light;
        }

        sleep_ms(100);
    }
}
