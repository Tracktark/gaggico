#include "protocol.hpp"
#include <pico/time.h>
#include <pico/mutex.h>
#include <hardware/watchdog.h>
#include "control/control.hpp"
#include "control/impl/state_machine.hpp"
#include "control/states.hpp"
#include "network/network.hpp"
#include "network/messages.hpp"
#include "ntp.hpp"

using namespace protocol;

MachineState _state;

auto_init_mutex(core1_alive_mutex);
static volatile bool core1_alive = false;

int protocol::get_state_id() {
    return statemachine::curr_state_id;
}

void protocol::on_state_change(int old_state_id, int new_state_id) {
    _state.state_change_time = get_absolute_time();

    StateChangeMessage msg;
    msg.new_state = new_state_id;
    msg.state_change_timestamp = ntp::to_timestamp(_state.state_change_time) / 1000;
    msg.machine_start_timestamp = ntp::to_timestamp(_state.machine_start_time) / 1000;
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
        // Only update watchdog if core1 is also alive
        if (mutex_try_enter(&core1_alive_mutex, nullptr)) {
            if (core1_alive) {
                watchdog_update();
                core1_alive = false;
            }
            mutex_exit(&core1_alive_mutex);
        }

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
        mutex_enter_blocking(&core1_alive_mutex);
        core1_alive = true;
        mutex_exit(&core1_alive_mutex);

        if (statemachine::curr_state_id == OffState::ID) continue;

        const control::Sensors& s = control::sensors();
        msg.pressure = s.pressure;
        msg.temp = s.temperature;
        network::send(msg);
        sleep_ms(statemachine::curr_state_id == BrewState::ID ? 100 : 250);
    }
}

MachineState& protocol::state() {
    return _state;
}
