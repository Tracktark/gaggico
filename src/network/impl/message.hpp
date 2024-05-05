#pragma once

#include <variant>
#include <boost/pfr.hpp>
#include "serde.hpp"
#include "inttypes.hpp"

template <typename T>
concept IncomingMsg = requires(T t) {
    { T::INCOMING_ID } -> std::convertible_to<i32>;
    { t.handle() } -> std::same_as<void>;
};

template <typename Tuple, typename Variant>
struct handle_impl;

template <IncomingMsg T, IncomingMsg... Ts, typename Variant>
struct handle_impl<std::variant<T, Ts...>, Variant> {
    static void handle(int msg_id, Variant& msg, u8*& data) {
        if (msg_id == T::INCOMING_ID) {
            T& t = msg.template emplace<T>();
            read_struct<T>(t, data);
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
void handle_incoming_msg(int msg_id, u8*& data_ptr) {
    Variant msg;
    handle_impl<Variant, Variant>::handle(msg_id, msg, data_ptr);
}

template <typename T>
concept OutgoingMsg = requires(T t) {
    { T::OUTGOING_ID } -> std::convertible_to<i32>;
};
