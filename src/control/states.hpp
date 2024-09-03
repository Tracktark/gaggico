#pragma once

#include <pico/time.h>
#include "control.hpp"
#include "control/protocol.hpp"
#include "hardware/hardware.hpp"
#include "impl/coroutine.hpp"
#include "impl/state_machine.hpp"
#include "messages.hpp"
#include "settings.hpp"

template <int id>
using State = statemachine::State<id>;

namespace states {
extern MaintenanceStatusMessage maintenance_msg;
};

struct OffState : State<0> {
    static void on_enter() {
        control::set_pump_enabled(false);
        control::set_boiler_enabled(false);
        hardware::set_solenoid(false);
        hardware::set_light(hardware::Power, false);
        hardware::set_light(hardware::Brew, false);
        hardware::set_light(hardware::Steam, false);
    }

    static void on_exit() {
        auto& state = protocol::state();
        state.machine_start_time = get_absolute_time();
        state.cold_start = hardware::read_temp() < 70;
        hardware::set_light(hardware::Power, true);
        control::reset();
    }

    static bool check_transitions();
};

struct StandbyState : State<1> {
    static void on_enter() {
        const Settings& s = settings::get();
        control::set_pid_params(s.kp, s.ki, s.kd);
        control::set_boiler_enabled(true);
        control::set_target_temperature(settings::get().brew_temp);
    }

    static void on_exit() {
        hardware::set_pump(0);
        hardware::set_solenoid(false);
        control::set_light_blink(0);
    }

    static bool check_transitions();
    static Protocol protocol();
};

struct BrewState : State<2> {
    static void on_enter() {
        protocol::state().brew_start_time = get_absolute_time();
        hardware::set_solenoid(true);
        control::set_pump_enabled(true);
    }

    static void on_exit() {
        control::set_pump_enabled(false);
        hardware::set_solenoid(false);
        control::set_light_blink(0);
    }

    static bool check_transitions();
    static Protocol protocol();
};

struct SteamState : State<3> {
    static void on_enter() {
        control::set_target_temperature(settings::get().steam_temp);
        control::set_pump_enabled(false);
        control::set_boiler_enabled(false);
    }

    static void on_exit() {
        hardware::set_pump(0);
        hardware::set_heater(0);
        control::set_light_blink(0);
    }

    static bool check_transitions();
    static Protocol protocol();
};

struct BackflushState : State<4> {
    static void on_enter() {
        states::maintenance_msg.cycle = 1;
        states::maintenance_msg.stage = 0;
    }

    static void on_exit() {
        control::set_pump_enabled(false);
        hardware::set_solenoid(false);
        control::set_light_blink(0);
    }
    static Protocol protocol();
};

struct DescaleState : State<5> {
    static void on_enter() {
        states::maintenance_msg.cycle = 1;
        states::maintenance_msg.stage = 0;
        control::set_boiler_enabled(false);
        control::set_light_blink(1000);
        hardware::set_solenoid(false);
    }

    static void on_exit() {
        hardware::set_pump(0);
        control::set_light_blink(0);
        hardware::set_solenoid(false);
    }
    static Protocol protocol();
};

using States = std::tuple<OffState, StandbyState, BrewState, SteamState, BackflushState, DescaleState>;
