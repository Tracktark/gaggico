#include "messages.hpp"
#include "control/protocol.hpp"
#include "network.hpp"
#include "ntp.hpp"
#include "settings.hpp"

void GetStatusMessage::handle() {
    StateChangeMessage msg;
    msg.new_state = (int)protocol::get_state();
    msg.machine_start_timestamp = ntp::to_timestamp(protocol::get_machine_start_time()) / 1000;
    msg.state_change_timestamp = ntp::to_timestamp(protocol::get_state_change_time()) / 1000;
    network::send(msg);
    network::send(SettingsGetMessage());
}
