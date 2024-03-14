#include "protocol.hpp"
#include <pico/time.h>
#include <hardware/watchdog.h>
#include "control/control.hpp"
#include "control/impl/state_machine.hpp"
#include "control/states.hpp"
#include "network/network.hpp"
#include "ntp.hpp"

using namespace protocol;

Times _times;

int protocol::get_state_id() {
    return statemachine::curr_state_id;
}

void protocol::on_state_change(int old_state_id, int new_state_id) {
    _times.state_change = get_absolute_time();

    StateChangeMessage msg;
    msg.new_state = new_state_id;
    msg.state_change_timestamp = ntp::to_timestamp(_times.state_change) / 1000;
    msg.machine_start_timestamp = ntp::to_timestamp(_times.machine_start) / 1000;
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
        watchdog_update();
        bool should_restart = statemachine::curr_state_check_transitions();
        if (should_restart) continue;

        if (hardware::is_power_just_pressed()) {
            statemachine::change_state<OffState>();
            continue;
        }

        control::update_sensors();

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
        if (statemachine::curr_state_id == OffState::ID) continue;
        const control::Sensors& s = control::sensors();
        msg.pressure = s.pressure;
        msg.temp = s.temperature;
        network::send(msg);
        sleep_ms(250);
    }
}

Times& protocol::times() {
    return _times;
}
