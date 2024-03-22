#pragma once

#include <pico/time.h>
#include "control.hpp"
#include "control/protocol.hpp"
#include "hardware/hardware.hpp"
#include "impl/coroutine.hpp"
#include "impl/state_machine.hpp"
#include "settings.hpp"

template <int id>
using State = statemachine::State<id>;

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
        protocol::times().machine_start = get_absolute_time();
        hardware::set_light(hardware::Power, true);
    }

    static bool check_transitions();
};

struct StandbyState : State<1> {
    static void on_enter() {
        const Settings& s = settings::get();
        control::set_pid_params(s.kp, s.ki, s.kd);
        control::reset();
        control::set_boiler_enabled(true);
        control::set_target_temperature(settings::get().brew_temp);
    }

    static void on_exit() {
        hardware::set_pump(0);
        hardware::set_solenoid(false);
    }

    static bool check_transitions();
    static Protocol protocol();
};

struct BrewState : State<2> {
    static void on_enter() {
        protocol::times().brew_start = get_absolute_time();
        hardware::set_solenoid(true);
        control::set_pump_enabled(true);
    }

    static void on_exit() {
        control::set_pump_enabled(false);
        hardware::set_solenoid(false);
    }

    static bool check_transitions();
    static Protocol protocol();
};

struct SteamState : State<3> {
    static void on_enter() {
        control::set_target_temperature(settings::get().steam_temp);
    }

    static void on_exit() {
        hardware::set_light(hardware::Steam, false);
        hardware::set_solenoid(false);
    }

    static bool check_transitions();
    static Protocol protocol();
};
