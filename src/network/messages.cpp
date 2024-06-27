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
    network::send(msg);
    network::send(SettingsGetMessage());

void MaintenanceMessage::handle() {
    if (statemachine::curr_state_id == StandbyState::ID) {
        if (type == 1) { // Backflush
            statemachine::change_state<BackflushState>();
        } else if (type == 2) { // Descale
            statemachine::change_state<DescaleState>();
        }
    } else if (statemachine::curr_state_id == BackflushState::ID ||
               statemachine::curr_state_id == DescaleState::ID) {
        if (type == 0) { // Stop
            statemachine::change_state<StandbyState>();
        }
    }
}
