#include "messages.hpp"
#include "control/protocol.hpp"
#include "network.hpp"
#include "settings.hpp"

void GetStatusMessage::handle() {
    StateChangeMessage msg;
    msg.new_state = (int)protocol::get_state();
    network::send(msg);
    network::send(SettingsGetMessage());
}
