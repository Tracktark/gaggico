#pragma once
#include <bit>
#include <lwip/def.h>
#include <boost/pfr.hpp>
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

template <typename T>
concept CustomRead = requires(T t, u8*& ptr) {
    { t.read(ptr) } -> std::same_as<void>;
};

template <typename T, std::size_t FieldCount, std::size_t CurrField>
struct read_impl {
    static void read(T& val, u8*& data_ptr) {
        read_val(data_ptr, boost::pfr::get<FieldCount-CurrField>(val));
        read_impl<T, FieldCount, CurrField-1>::read(val, data_ptr);
    }
};

template <typename T, std::size_t FieldCount>
struct read_impl<T, FieldCount, 0> {
    static void read(T& val, u8*& data_ptr) {}
};

template <CustomRead T>
void read_struct(T& val, u8*& data_ptr) {
    val.read(data_ptr);
}

template <typename T>
void read_struct(T& val, u8*& data_ptr) {
    constexpr std::size_t field_count = boost::pfr::tuple_size_v<T>;
    read_impl<T, field_count, field_count>::read(val, data_ptr);
}

template <typename T>
concept CustomWrite = requires(const T& t, u8*& ptr) {
    { t.write(ptr) } -> std::same_as<void>;
};

template <typename T, std::size_t FieldCount, std::size_t CurrField>
struct write_impl {
    static void write(const T& val, u8*& data_ptr) {
        write_val(data_ptr, boost::pfr::get<FieldCount-CurrField>(val));
        write_impl<T, FieldCount, CurrField-1>::write(val, data_ptr);
    }
};

template <typename T, std::size_t FieldCount>
struct write_impl<T, FieldCount, 0> {
    static void write(const T& val, u8*& data_ptr) {}
};


template <CustomWrite T>
void write_struct(const T& val, u8*& data_ptr) {
    val.write(data_ptr);
}

template <typename T>
void write_struct(const T& val, u8*& data_ptr) {
    constexpr std::size_t field_count = boost::pfr::tuple_size_v<T>;
    write_impl<T, field_count, field_count>::write(val, data_ptr);
}
