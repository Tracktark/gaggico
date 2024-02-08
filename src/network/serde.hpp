#pragma once
#include <bit>
#include <lwip/def.h>
#include "inttypes.hpp"

template <class>
constexpr bool dependent_false = false;
template <typename T>
inline void write_val(u8*& ptr, T value) {
    if constexpr (sizeof(T) == 8) {
        if constexpr (std::endian::native == std::endian::little) {
            *reinterpret_cast<u32*>(ptr) = htonl(*(reinterpret_cast<u32*>(&value) + 1));
            *(reinterpret_cast<u32*>(ptr) + 1) = htonl(*reinterpret_cast<u32*>(&value));
        } else {
            *reinterpret_cast<u64*>(ptr) = *reinterpret_cast<u64*>(&value);
        }
        ptr += 8;
    } else if constexpr (sizeof(T) == 4) {
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
inline void read_val(u8*& ptr, T& value) {
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
