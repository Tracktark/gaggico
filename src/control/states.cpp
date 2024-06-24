#include "states.hpp"
#include <pico/time.h>
#include "control.hpp"
#include "impl/coroutine.hpp"
#include "protocol.hpp"
#include "hardware/hardware.hpp"
#include "settings.hpp"

#define us_since(time) (absolute_time_diff_us((time), get_absolute_time()))

bool OffState::check_transitions() {
    if (hardware::is_power_just_pressed()) {
        statemachine::change_state<StandbyState>();
    }
    return true;
}

bool StandbyState::check_transitions() {
    if (hardware::get_switch(hardware::Steam)) {
        statemachine::change_state<SteamState>();
        return true;
    }
    if (hardware::get_switch(hardware::Brew)) {
        statemachine::change_state<BrewState>();
        return true;
    }
    return false;
}

Protocol StandbyState::protocol() {
    if (us_since(protocol::times().machine_start) > 10'000)
        co_return;
    if (control::sensors().temperature > 80)
        co_return;

    hardware::set_pump(0.5);
    hardware::set_solenoid(true);

    co_await delay_ms(200);

    constexpr auto MAX_PREFILL_TIME_MS = 12000;
    absolute_time_t timeout = make_timeout_time_ms(MAX_PREFILL_TIME_MS);

    while (true) {
        co_await delay_ms(10);

        if (time_reached(timeout)) break;

        float pressure = control::sensors().pressure;
        if (pressure > 1.20f) break;
    }

    hardware::set_pump(0);
    hardware::set_solenoid(false);
}

bool BrewState::check_transitions() {
    if (!hardware::get_switch(hardware::Brew)) {
        statemachine::change_state<StandbyState>();
        return true;
    }
    return false;
}
Protocol BrewState::protocol() {
    control::set_target_pressure(settings::get().preinfusion_pressure);

    co_await delay_ms(settings::get().preinfusion_time * 1000);

    control::set_target_pressure(settings::get().brew_pressure);

    float brew_time = settings::get().brew_time;
    if (brew_time < 0) co_return;

    co_await delay_ms(brew_time * 1000);

    if (!hardware::get_switch(hardware::Steam)) {
        hardware::set_solenoid(false);
        control::set_pump_enabled(false);
    }

    bool light_on = false;
    bool pump_on = false;
    absolute_time_t blink_timeout = nil_time;
    while (true) {
        if (time_reached(blink_timeout)) {
            hardware::set_light(hardware::Steam, light_on);
            light_on = !light_on;
            blink_timeout = make_timeout_time_ms(250);
        }

        if (hardware::get_switch(hardware::Steam) != pump_on) {
            pump_on = hardware::get_switch(hardware::Steam);
            hardware::set_solenoid(pump_on);
            control::set_pump_enabled(pump_on);
        }
        co_await next_cycle;
    }
}

bool SteamState::check_transitions() {
    if (!hardware::get_switch(hardware::Steam)) {
        statemachine::change_state<StandbyState>();
        return true;
    }
    return false;
}
Protocol SteamState::protocol() {
    const control::Sensors& sensors = control::sensors();

    co_await predicate([] {
        return control::sensors().temperature > 120;
    });

    hardware::set_solenoid(true);
    co_await delay_ms(1500);
    hardware::set_solenoid(false);

    co_await predicate([] {
        return control::sensors().temperature > 140;
    });

    hardware::set_light(hardware::Steam, true);

    float max_pressure = sensors.pressure;
    while (true) {
        co_await delay_ms(50);

        if (sensors.pressure > max_pressure) {
            max_pressure = sensors.pressure;
        }

        if (max_pressure - sensors.pressure > 2) {
            break;
        }
    }

    while (true) {
        hardware::set_light(hardware::Steam, false);
        co_await delay_ms(250);
        hardware::set_light(hardware::Steam, true);
        co_await delay_ms(250);
    }

}
