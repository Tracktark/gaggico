#include "protocol.hpp"
#include <cstdio>
#include <pico/time.h>
#include <pico/mutex.h>
#include <hardware/watchdog.h>
#include "control/control.hpp"
#include "control/impl/state_machine.hpp"
#include "control/states.hpp"
#include "network/network.hpp"
#include "network/messages.hpp"
#include "network/ntp.hpp"

using namespace protocol;

MachineState _state;

auto_init_mutex(core1_alive_mutex);
static volatile bool core1_alive = false;
static volatile bool core1_watchdog_enabled = false;

auto_init_mutex(next_state_mutex);
static volatile int next_state = -1;

int protocol::get_state_id() {
    return statemachine::curr_state_id;
}

void protocol::schedule_state_change_by_id(int id) {
    mutex_enter_blocking(&next_state_mutex);
    next_state = id;
    mutex_exit(&next_state_mutex);
}

void protocol::on_state_change(int old_state_id, int new_state_id) {
    _state.state_change_time = get_absolute_time();

    StateChangeMessage msg;
    msg.new_state = new_state_id;
    msg.state_change_timestamp = ntp::to_timestamp(_state.state_change_time) / 1000;
    msg.machine_start_timestamp = ntp::to_timestamp(_state.machine_start_time) / 1000;
    network::enqueue_message(msg);
}

void protocol::set_power(bool on) {
    if (statemachine::curr_state_id == OffState::ID && on) {
        schedule_state_change<StandbyState>();
    } else if (statemachine::curr_state_id != OffState::ID && !on){
        schedule_state_change<OffState>();
    }
}

void protocol::main_loop() {
    statemachine::enter_state<OffState>();
    state().last_loop_time = get_absolute_time();

    while (true) {
        absolute_time_t now = get_absolute_time();
        uint32_t loop_time = absolute_time_diff_us(state().last_loop_time, now);
        if (loop_time > 10'000) {
            printf("Loop time longer than 10 ms! %.02f ms\n", loop_time / 1000.0);
        }
        state().last_loop_time = now;

        // Only update watchdog if core1 is also alive
        if (mutex_try_enter(&core1_alive_mutex, nullptr)) {
            if (!core1_watchdog_enabled || core1_alive) {
                watchdog_update();
                core1_alive = false;
            }
            mutex_exit(&core1_alive_mutex);
        }

        // Process scheduled state change
        if (mutex_try_enter(&next_state_mutex, nullptr)) {
            if (next_state >= 0) {
                statemachine::change_state_by_id<States>(next_state);
                next_state = -1;
                mutex_exit(&next_state_mutex);
                continue;
            } else {
                mutex_exit(&next_state_mutex);
            }
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
    absolute_time_t sensor_message_time = nil_time;

    // Enable watchdog
    mutex_enter_blocking(&core1_alive_mutex);
    core1_watchdog_enabled = true;
    mutex_exit(&core1_alive_mutex);

    while (true) {
        sleep_ms(10);

        // Update watchdog
        mutex_enter_blocking(&core1_alive_mutex);
        core1_alive = true;
        mutex_exit(&core1_alive_mutex);

        network::process_outgoing_messages();

        if (get_state_id() != OffState::ID && time_reached(sensor_message_time)) {
            sensor_message_time = make_timeout_time_ms(get_state_id() == BrewState::ID ? 100 : 250);

            if (network::message_queue_size() < 5) {
                const control::Sensors& s = control::sensors();
                msg.pressure = s.pressure;
                msg.temp = s.temperature;
                network::enqueue_message(msg);
            }
        }
    }
}

MachineState& protocol::state() {
    return _state;
}
