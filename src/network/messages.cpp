#include "messages.hpp"
#include "control/protocol.hpp"
#include "control/states.hpp"
#include "network.hpp"
#include "ntp.hpp"

void GetStatusMessage::handle() {
    StateChangeMessage msg;
    msg.new_state = protocol::get_state_id();
    msg.machine_start_timestamp = ntp::to_timestamp(protocol::state().machine_start_time) / 1000;
    msg.state_change_timestamp = ntp::to_timestamp(protocol::state().state_change_time) / 1000;
    network::enqueue_message(msg);
    network::enqueue_message(SettingsGetMessage());

    if (protocol::get_state_id() == BackflushState::ID ||
        protocol::get_state_id() == DescaleState::ID) {
        network::enqueue_message(states::maintenance_msg);
    }
}

void MaintenanceMessage::handle() {
    if (protocol::get_state_id() == StandbyState::ID) {
        if (type == 1) { // Backflush
            protocol::schedule_state_change<BackflushState>();
        } else if (type == 2) { // Descale
            protocol::schedule_state_change<DescaleState>();
        }
    } else if (protocol::get_state_id() == BackflushState::ID ||
               protocol::get_state_id() == DescaleState::ID) {
        if (type == 0) { // Stop
            protocol::schedule_state_change<StandbyState>();
        }
    }
}

void ManualControlMessage::handle() {
    if (protocol::get_state_id() == StandbyState::ID) {
        ManualControlState::target_flow = target_flow;
        ManualControlState::target_pressure = target_pressure;
        ManualControlState::time_ms = time_ms;
        protocol::schedule_state_change<ManualControlState>();
    } else if (protocol::get_state_id() == ManualControlState::ID) {
        protocol::schedule_state_change<StandbyState>();
    }
}
