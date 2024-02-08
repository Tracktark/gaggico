#include "protocol.hpp"
#include <pico/time.h>
#include "control.hpp"
#include "hardware/hardware.hpp"
#include "network/network.hpp"
#include "ntp.hpp"
#include "settings.hpp"

using namespace protocol;

State current_state = State::Off;
absolute_time_t brew_start_time;
absolute_time_t machine_start_time;
absolute_time_t state_change_time;

State protocol::get_state() {
    return current_state;
}

void on_enter_state(State state) {
    switch (state) {
    case State::Off:
        control::set_pump_enabled(false);
        control::set_boiler_enabled(false);
        hardware::set_solenoid(false);
        hardware::set_light(hardware::Power, false);
        hardware::set_light(hardware::Brew, false);
        hardware::set_light(hardware::Steam, false);
        break;
    case State::Standby:
        control::set_boiler_enabled(true);
        break;
    case State::Brew:
        brew_start_time = get_absolute_time();
        hardware::set_solenoid(true);
        control::set_pump_enabled(true);
    }
}

void on_leave_state(State state) {
    switch (state) {
    case State::Brew:
        control::set_pump_enabled(false);
        hardware::set_solenoid(false);
        break;
    case State::Off:
        hardware::set_light(hardware::Power, true);
        break;
    case State::Standby:
        break;
    }
}

void switch_state(State new_state) {
    if (current_state == new_state) return;

    on_leave_state(current_state);
    current_state = new_state;
    on_enter_state(current_state);
    state_change_time = get_absolute_time();

    StateChangeMessage msg;
    msg.new_state = (int)new_state;
    msg.state_change_timestamp = ntp::to_timestamp(state_change_time) / 1000;
    msg.machine_start_timestamp = ntp::to_timestamp(machine_start_time) / 1000;
    network::send(msg);
}

void protocol::set_power(bool on) {
    if (current_state == State::Off && on) {
        machine_start_time = get_absolute_time();
        switch_state(State::Standby);
    } else if (current_state != State::Off && !on){
        switch_state(State::Off);
    }
}

void protocol::main_loop() {
    on_enter_state(State::Off);

    while (true) {
        if (current_state == State::Off) {
            if (hardware::is_power_just_pressed()) {
                set_power(true);
            }
            continue;
        }

        if (hardware::is_power_just_pressed()) {
            set_power(false);
            continue;
        }

        if (current_state == State::Standby) {
            if (hardware::get_switch(hardware::Brew)) {
                switch_state(State::Brew);
                continue;
            }

            if (absolute_time_diff_us(machine_start_time, get_absolute_time()) < 2'000'000) {
                hardware::set_pump(0.35);
                hardware::set_solenoid(true);
            } else {
                control::set_pump_enabled(false);
                hardware::set_solenoid(false);
            }

            if (hardware::get_switch(hardware::Steam)) {
                control::set_target_temperature(settings::get().steam_temp);
            } else {
                control::set_target_temperature(settings::get().brew_temp);
            }
        }

        if (current_state == State::Brew) {
            if (!hardware::get_switch(hardware::Brew)) {
                switch_state(State::Standby);
                continue;
            }
            if (absolute_time_diff_us(brew_start_time, get_absolute_time()) < settings::get().preinfusion_time * 1'000'000) {
                control::set_target_pressure(settings::get().preinfusion_pressure);
            } else {
                control::set_target_pressure(settings::get().brew_pressure);
            }
        }

        control::update();
    }
}

void protocol::network_loop() {
    SensorStatusMessage msg;
    while (true) {
        msg.pressure = hardware::read_pressure();
        msg.temp = hardware::read_temp();
        network::send(msg);
        sleep_ms(250);
    }
}

absolute_time_t protocol::get_machine_start_time() {
    return machine_start_time;
}

absolute_time_t protocol::get_state_change_time() {
    return state_change_time;
}
