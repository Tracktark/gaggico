#include "states.hpp"
#include <cmath>
#include <pico/time.h>
#include "control.hpp"
#include "impl/coroutine.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "hardware/hardware.hpp"
#include "settings.hpp"

#define us_since(time) (absolute_time_diff_us((time), get_absolute_time()))
#define ms_since(time) (absolute_time_diff_us((time), get_absolute_time()) / 1000)

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
        if (pressure > 0.42f) break;
    }

    hardware::set_pump(0);
    hardware::set_solenoid(false);
    co_await delay_ms(2000);
    hardware::scale_start_tare();
}

bool BrewState::check_transitions() {
    if (!hardware::get_switch(hardware::Brew)) {
        statemachine::change_state<StandbyState>();
        return true;
    }

    const control::Sensors& s = control::sensors();
    const Settings set = settings::get();
    if (hardware::is_scale_connected() && s.weight < 0 && s.weight > -5 && !hardware::is_scale_taring()) {
        hardware::scale_tare_immediately();
    }
    return false;
}
Protocol BrewState::protocol() {
    bool has_scales = hardware::is_scale_connected();
    bool tare_started = false;
    bool tare_done = false;
    bool preinfusion_done = false;
    absolute_time_t start = get_absolute_time();

    u32 preinfusion_ms = settings::get().preinfusion_time * 1000;
    u32 ms_before_tare = fmin(fmax(preinfusion_ms - 500, 0), 1000);
    float brew_weight = settings::get().brew_weight;

    control::set_target_pressure(settings::get().preinfusion_pressure);

    while (true) {
        if (has_scales && !tare_started && ms_since(start) > ms_before_tare) {
            tare_started = true;
            hardware::scale_start_tare();
        }
        if (has_scales && tare_started && !tare_done && !hardware::is_scale_taring()) {
            tare_done = true;
        }
        if (ms_since(start) > preinfusion_ms && !preinfusion_done) {
            preinfusion_done = true;
            control::set_target_pressure(settings::get().brew_pressure);
        }
        if (tare_done && brew_weight > 0 && control::sensors().weight >= brew_weight) {
            break;
        }

        co_await next_cycle;
    }

    hardware::set_solenoid(false);
    control::set_pump_enabled(false);

    control::set_light_blink(250);
}

bool SteamState::check_transitions() {
    if (!hardware::get_switch(hardware::Steam)) {
        statemachine::change_state<StandbyState>();
        return true;
    }
    return false;
}
Protocol SteamState::protocol() {
    hardware::set_heater(1);
    co_await predicate([] {
        return control::sensors().temperature > settings::get().steam_temp;
    });
    hardware::set_light(hardware::Steam, true);

    const control::Sensors& sensors = control::sensors();
    const Settings &settings = settings::get();
    constexpr float PRESSURE_TARGET = 4;

    bool valve_open = false;
    float max_pressure = sensors.pressure;
    float min_pressure = sensors.pressure;

    while (true) {
        // Detect valve open
        if (!valve_open) {
            if (sensors.pressure > max_pressure && sensors.pressure < 6) {
                max_pressure = sensors.pressure;
            }
            if (max_pressure - sensors.pressure > 1) {
                valve_open = true;
                hardware::set_pump(0.05);
                control::set_light_blink(250);
                min_pressure = sensors.pressure;
            }
        } else {
            if (sensors.pressure < min_pressure) {
                min_pressure = sensors.pressure;
            }
            if (sensors.pressure - min_pressure > 1) {
                hardware::set_pump(0);
                valve_open = false;
                control::set_light_blink(0);
                hardware::set_light(hardware::Steam, true);
                max_pressure = sensors.pressure;
            }
        }

        if (sensors.temperature < settings.steam_temp) {
            hardware::set_heater(1);
        } else {
            hardware::set_heater(0);
        }

        co_await next_cycle;
    }
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
