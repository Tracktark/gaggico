#include "protocol.hpp"
#include <pico/time.h>
#include "control.hpp"
#include "hardware/hardware.hpp"
#include "control/impl/state_machine.hpp"
#include "control/states.hpp"
#include "network/network.hpp"
#include "ntp.hpp"

using namespace protocol;

absolute_time_t brew_start_time;
absolute_time_t machine_start_time;
absolute_time_t state_change_time;

int protocol::get_state_id() {
    return statemachine::curr_state_id;
}

void protocol::on_state_change(int old_state_id, int new_state_id) {
    _times.state_change = get_absolute_time();

    StateChangeMessage msg;
    msg.state_change_timestamp = ntp::to_timestamp(state_change_time) / 1000;
    msg.machine_start_timestamp = ntp::to_timestamp(machine_start_time) / 1000;
    msg.new_state = new_state_id;
    network::send(msg);
}

void protocol::set_power(bool on) {
    if (statemachine::curr_state_id == OffState::ID && on) {
        statemachine::change_state<StandbyState>();
    } else if (statemachine::curr_state_id != OffState::ID && !on){
        statemachine::change_state<OffState>();
    }
}

void protocol::main_loop() {
    statemachine::enter_state<OffState>();

    while (true) {
        bool should_restart = statemachine::curr_state_check_transitions();
        if (should_restart) continue;

        if (hardware::is_power_just_pressed()) {
            statemachine::change_state<OffState>();
            continue;
        }


        if (Protocol::has_coroutine) {
            Protocol::Handle handle = Protocol::handle();
            if (!Protocol::curr_awaiter || Protocol::curr_awaiter->should_resume()) {
                handle.resume();
                if (handle.done()) {
                    handle.destroy();
                }
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
