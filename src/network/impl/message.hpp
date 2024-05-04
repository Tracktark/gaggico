#pragma once

#include <variant>
#include <boost/pfr.hpp>
#include "inttypes.hpp"

template <typename T>
concept IncomingMsg = requires(T t) {
    { T::INCOMING_ID } -> std::convertible_to<i32>;
    { t.handle() } -> std::same_as<void>;
};

template <typename T>
concept CustomReadMsg = IncomingMsg<T> && requires(T t, u8*& ptr) {
    { t.read(ptr) } -> std::same_as<void>;
};

template <IncomingMsg Msg, std::size_t FieldCount, std::size_t CurrField>
struct read_impl {
    static void read(Msg& msg, u8*& data_ptr) {
        read_val(data_ptr, boost::pfr::get<FieldCount-CurrField>(msg));
        read_impl<Msg, FieldCount, CurrField-1>::read(msg, data_ptr);
    }
};

template <IncomingMsg Msg, std::size_t FieldCount>
struct read_impl<Msg, FieldCount, 0> {
    static void read(Msg& msg, u8*& data_ptr) {}
};


template <CustomReadMsg Msg>
void read(Msg& msg, u8*& data_ptr) {
    msg.read(data_ptr);
}

template <typename Msg>
void read(Msg& msg, u8*& data_ptr) {
    constexpr std::size_t field_count = boost::pfr::tuple_size_v<Msg>;
    read_impl<Msg, field_count, field_count>::read(msg, data_ptr);
}


template <typename Tuple, typename Variant>
struct handle_impl;

template <IncomingMsg T, IncomingMsg... Ts, typename Variant>
struct handle_impl<std::variant<T, Ts...>, Variant> {
    static void handle(int msg_id, Variant& msg, u8*& data) {
        if (msg_id == T::INCOMING_ID) {
            T& t = msg.template emplace<T>();
            read<T>(t, data);
            t.handle();
        } else {
            handle_impl<std::variant<Ts...>, Variant>::handle(msg_id, msg, data);
        }
    }
};

template <typename Variant>
struct handle_impl<std::variant<>, Variant> {
    static void handle(int msg_id, Variant& msg, u8*& data) {}
};

template <typename Variant>
void handle(int msg_id, u8*& data_ptr) {
    Variant msg;
    handle_impl<Variant, Variant>::handle(msg_id, msg, data_ptr);
}
