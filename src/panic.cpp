#include "panic.hpp"
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include "hardware.hpp"
#include "inttypes.hpp"

constexpr auto DELAY_SHORT = 500;
constexpr auto DELAY_LONG = DELAY_SHORT * 5;

[[noreturn]]
void panic(Error error) {
    if (get_core_num() == 1) {
        multicore_lockout_start_blocking();
    }
    u32 blinks = static_cast<u32>(error) + 1;
    while (true) {
        hardware::set_heater(0);
        hardware::set_pump(0);
        hardware::set_solenoid(false);
        for (int i = 0; i < blinks; i++) {
            hardware::set_light(hardware::Power, true);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
            sleep_ms(DELAY_SHORT);
            hardware::set_light(hardware::Power, false);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
            sleep_ms(DELAY_SHORT);
        }
        sleep_ms(DELAY_LONG - DELAY_SHORT);
    }
}
