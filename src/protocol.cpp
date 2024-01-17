#include "protocol.hpp"
#include <pico/time.h>
#include "control.hpp"
#include "hardware.hpp"
#include "network.hpp"
#include "settings.hpp"

using namespace protocol;

State current_state = State::Off;

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
        hardware::set_solenoid(true);
        control::set_target_pressure(9);
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

    StateChangeMessage msg;
    msg.new_state = (int)new_state;
    network::send(msg);
}

void protocol::set_power(bool on) {
    if (current_state == State::Off && on) {
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
