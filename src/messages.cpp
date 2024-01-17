#include "messages.hpp"
#include "serde.hpp"

void StateChangeMessage::write(u8*& ptr) {
    write_val(ptr, ID);
    write_val(ptr, new_state);
}

void SensorStatusMessage::write(u8*& ptr) {
    write_val(ptr, ID);
    write_val(ptr, temp);
    write_val(ptr, pressure);
}
