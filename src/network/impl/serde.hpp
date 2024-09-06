#pragma once
#include <bit>
#include <cstring>
#include <lwip/def.h>
#include <boost/pfr.hpp>
#include "inttypes.hpp"

template <class>
constexpr bool dependent_false = false;
template <typename T>
inline void write_val(u8*& ptr, T value) {
    if constexpr (sizeof(T) == 8) {
        if constexpr (std::endian::native == std::endian::little) {
            u32 val1, val2;
            memcpy(&val1, &value, sizeof(u32));
            memcpy(&val2, reinterpret_cast<u32 *>(&value) + 1, sizeof(u32));
            val1 = htonl(val1);
            val2 = htonl(val2);
            memcpy(ptr, &val2, sizeof(u32));
            memcpy(ptr + sizeof(u32), &val1, sizeof(u32));
        } else {
            memcpy(ptr, &value, sizeof(u64));
        }
        ptr += 8;
    } else if constexpr (sizeof(T) == 4) {
        u32 val;
        memcpy(&val, &value, sizeof(value));
        val = htonl(val);
        memcpy(ptr, &val, sizeof(val));
        ptr += sizeof(val);
    } else if constexpr (sizeof(T) == 2) {
        u16 val;
        memcpy(&val, &value, sizeof(value));
        val = htonl(val);
        memcpy(ptr, &val, sizeof(val));
        ptr += sizeof(val);
    } else if constexpr (sizeof(T) == 1) {
        memcpy(ptr, &value, 1);
        ptr++;
    } else {
        static_assert(dependent_false<T>, "write_val not implemented for this type");
    }
};

template <typename T>
inline void read_val(u8*& ptr, T& value) {
    if constexpr (sizeof(T) == 4) {
        u32 val;
        memcpy(&val, ptr, sizeof(T));
        val = ntohl(val);
        memcpy(&value, &val, sizeof(T));
        ptr += sizeof(T);
    } else if constexpr (sizeof(T) == 2) {
        u32 val;
        memcpy(&val, ptr, sizeof(T));
        val = ntohs(val);
        memcpy(&value, &val, sizeof(T));
        ptr += sizeof(T);
    } else if constexpr (sizeof(T) == 1) {
        memcpy(&value, ptr, 1);
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
