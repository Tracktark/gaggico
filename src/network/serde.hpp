#pragma once
#include <lwip/def.h>
#include "inttypes.hpp"

template <class>
constexpr bool dependent_false = false;
template <typename T>
void write_val(u8*& ptr, T value) {
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
        static_assert(dependent_false<T>, "write_val not implemented for this type");
    }
};

template <typename T>
void read_val(u8*& ptr, T& value) {
    if constexpr (sizeof(T) == 4) {
        *reinterpret_cast<u32*>(&value) = ntohl(*reinterpret_cast<u32*>(ptr));
        ptr += 4;
    } else if constexpr (sizeof(T) == 2) {
        *reinterpret_cast<u16*>(&value) = ntohs(*reinterpret_cast<u16*>(ptr));
        ptr += 2;
    } else if constexpr (sizeof(T) == 1) {
        *reinterpret_cast<u8*>(&value) = *ptr;
        ptr++;
    } else {
        static_assert(dependent_false<T>, "read_val not implemented for this type");
    }
}
