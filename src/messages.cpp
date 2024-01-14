#include "messages.hpp"
#include "lwip/def.h"

template <class>
constexpr bool dependent_false = false;
template <typename T>
static void write_val(u8*& ptr, T value) {
    if constexpr (sizeof(T) == 4) {
        *reinterpret_cast<u32*>(ptr) = htonl(*reinterpret_cast<u32*>(&value));
        ptr += 4;
    } else if constexpr (sizeof(T) == 2) {
        *reinterpret_cast<u16*>(ptr) = htons(*reinterpret_cast<u16*>(&value));
        ptr += 2;
    } else if constexpr (sizeof(T) == 1) {
        *ptr = *reinterpret_cast<u8*>(&value);
        ptr++;
    } else {
        static_assert(dependent_false<T>, "writeVal not implemented for this type");
    }
};

void StateChangeMessage::write(u8*& ptr) {
    write_val(ptr, ID);
    write_val(ptr, new_state);
}

void SensorStatusMessage::write(u8*& ptr) {
    write_val(ptr, ID);
    write_val(ptr, temp);
    write_val(ptr, pressure);
}
