#include "messages.hpp"
#include "control/protocol.hpp"
#include "network.hpp"
#include "ntp.hpp"
#include "settings.hpp"

void GetStatusMessage::handle() {
    StateChangeMessage msg;
    msg.machine_start_timestamp = ntp::to_timestamp(protocol::get_machine_start_time()) / 1000;
    msg.state_change_timestamp = ntp::to_timestamp(protocol::get_state_change_time()) / 1000;
    msg.new_state = protocol::get_state_id();
    network::send(msg);
    network::send(SettingsGetMessage());
}
