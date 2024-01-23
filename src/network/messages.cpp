#include "messages.hpp"
#include "network.hpp"
#include "control/protocol.hpp"
#include "serde.hpp"
#include "settings.hpp"

void StateChangeMessage::write(u8*& ptr) const {
    write_val(ptr, ID);
    write_val(ptr, new_state);
}

void SensorStatusMessage::write(u8*& ptr) const {
    write_val(ptr, ID);
    write_val(ptr, temp);
    write_val(ptr, pressure);
}

void SettingsGetMessage::write(u8*& ptr) const {
    write_val(ptr, ID);
    settings::get().write(ptr);
}

void PowerMessage::read(u8*& ptr) {
    read_val(ptr, status);
}
void PowerMessage::handle() {
    protocol::set_power(status);
}

void SettingsUpdateMessage::read(u8*& ptr) {
    settings.read(ptr);
}
void SettingsUpdateMessage::handle() {
    settings::update(settings);
    network::send(SettingsGetMessage());
}

void GetStatusMessage::read(u8*& ptr) {}
void GetStatusMessage::handle() {
    StateChangeMessage msg;
    msg.new_state = (int)protocol::get_state();
    network::send(msg);
    network::send(SettingsGetMessage());
}
