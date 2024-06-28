#include "states.hpp"
#include <pico/time.h>
#include "control.hpp"
#include "impl/coroutine.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "hardware/hardware.hpp"
#include "settings.hpp"

#define us_since(time) (absolute_time_diff_us((time), get_absolute_time()))

MaintenanceStatusMessage states::maintenance_msg;

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
    if (us_since(protocol::state().machine_start_time) > 100'000)
        co_return;
    if (!protocol::state().cold_start)
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

    bool pump_on = false;
    control::set_light_blink(250);

    while (true) {
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

    if (sensors.temperature < 120) {
        co_await predicate([] {
            return control::sensors().temperature > 120;
        });
        hardware::set_solenoid(true);
        co_await delay_ms(1500);
        hardware::set_solenoid(false);
    }


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

    control::set_light_blink(250);
}

Protocol BackflushState::protocol() {
    for (int j = 0; j < 2; j++) {
        control::set_light_blink(1000);
        states::maintenance_msg.stage = j;

        for (int i = 0; i < 5; i++) {
            states::maintenance_msg.cycle = i + 1;
            network::enqueue_message(states::maintenance_msg);

            control::set_pump_enabled(true);
            hardware::set_solenoid(true);
            control::set_target_pressure(settings::get().brew_pressure);

            co_await delay_ms(1000);

            float last_pressure = control::sensors().pressure;

            while (true) {
                co_await delay_ms(100);

                float pressure = control::sensors().pressure;
                if (pressure - last_pressure < 0) break;
                last_pressure = pressure;
            }

            co_await delay_ms(1000);

            control::set_pump_enabled(false);
            hardware::set_solenoid(false);
            control::set_target_pressure(settings::get().brew_pressure);

            co_await delay_ms(10000);
        }

        states::maintenance_msg.stage = 2;
        states::maintenance_msg.cycle = 0;
        network::enqueue_message(states::maintenance_msg);

        control::set_light_blink(250);
        if (j == 0) {
            co_await predicate([]() {return hardware::get_switch(hardware::Steam);});
        } else {
            co_await predicate([]() {return !hardware::get_switch(hardware::Steam);});
        }
    }
    protocol::schedule_state_change<StandbyState>();
}

Protocol DescaleState::protocol() {
    for (int cycle = 0; cycle < 7; cycle++) {
        // Cleaning
        states::maintenance_msg.stage = 0;
        states::maintenance_msg.cycle = cycle+1;
        network::enqueue_message(states::maintenance_msg);

        u32 total_time = 0;
        u32 wait_time = cycle < 6 ? 30'000 : 10'000;
        while (total_time < wait_time) {
            if (!hardware::get_switch(hardware::Steam)) {
                hardware::set_pump(1);
                total_time += 100;
            } else {
                hardware::set_pump(0);
            }
            co_await delay_ms(100);
        }

        // Soaking
        states::maintenance_msg.stage = 1;
        network::enqueue_message(states::maintenance_msg);
        hardware::set_pump(0);
        co_await delay_ms(cycle == 0 ? 1000 * 60 * 20 : 1000 * 60 * 3);
    }

    // Rinsing
    for (int rinse_cycle = 0; rinse_cycle < 8; rinse_cycle++) {
        states::maintenance_msg.cycle = rinse_cycle+1;
        states::maintenance_msg.stage = 3;
        network::enqueue_message(states::maintenance_msg);

        // Wait for tank refill
        control::set_light_blink(250);
        bool initial = hardware::get_switch(hardware::Brew);
        while (hardware::get_switch(hardware::Brew) == initial) {
            co_await next_cycle;
        }

        // Rinsing
        states::maintenance_msg.stage = 2;
        network::enqueue_message(states::maintenance_msg);
        control::set_light_blink(1000);

        hardware::set_solenoid(rinse_cycle % 2 == 1);
        u32 total_time = 0;
        while (total_time < 200'000) { // Time to empty entire tank
            if (!hardware::get_switch(hardware::Steam)) {
                hardware::set_pump(1);
                total_time += 100;
            } else {
                hardware::set_pump(0);
            }
            if (hardware::get_switch(hardware::Brew) == initial) {
                break;
            }
            co_await delay_ms(100);
        }
        hardware::set_pump(0);
        hardware::set_solenoid(false);
    }

    protocol::schedule_state_change<StandbyState>();
}
