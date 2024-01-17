#include "messages.hpp"
#include "protocol.hpp"
#include "serde.hpp"

void StateChangeMessage::write(u8*& ptr) const {
    write_val(ptr, ID);
    write_val(ptr, new_state);
}

void SensorStatusMessage::write(u8*& ptr) const {
    write_val(ptr, ID);
    write_val(ptr, temp);
    write_val(ptr, pressure);
}
void PowerMessage::read(u8*& ptr) {
    read_val(ptr, status);
}
void PowerMessage::handle() {
    protocol::set_power(status);
}

