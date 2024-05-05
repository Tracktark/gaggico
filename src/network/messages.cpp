#include "messages.hpp"
#include "control/protocol.hpp"
#include "network.hpp"
#include "ntp.hpp"

void GetStatusMessage::handle() {
    StateChangeMessage msg;
    msg.new_state = protocol::get_state_id();
    msg.machine_start_timestamp = ntp::to_timestamp(protocol::times().machine_start) / 1000;
    msg.state_change_timestamp = ntp::to_timestamp(protocol::times().state_change) / 1000;
    network::send(msg);
    network::send(SettingsGetMessage());
}
