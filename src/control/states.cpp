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
        control::set_target_temperature(settings::get().steam_temp);
    } else {
        control::set_target_temperature(settings::get().brew_temp);
    }
    if (hardware::get_switch(hardware::Brew)) {
        statemachine::change_state<BrewState>();
        return true;
    }
    return false;
}

Protocol StandbyState::protocol() {
    if (us_since(protocol::times().machine_start) > 1'000'000)
        co_return;

    hardware::set_pump(0.35);
    hardware::set_solenoid(true);

    co_await delay_ms(1000);

    float prev_pressure = control::sensors().pressure;
    constexpr auto MAX_PREFILL_TIME_MS = 8000;
    absolute_time_t timeout = make_timeout_time_ms(MAX_PREFILL_TIME_MS);

    while (true) {
        co_await delay_ms(10);

        if (time_reached(timeout)) break;

        float curr_pressure = control::sensors().pressure;
        float diff = prev_pressure - curr_pressure;
        prev_pressure = curr_pressure;
        if (diff > -0.02f && diff < 0.001f) break;
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
}
