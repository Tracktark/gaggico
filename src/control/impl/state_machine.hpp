#pragma once

#include <concepts>
#include "coroutine.hpp"
#include "control/protocol.hpp"

namespace statemachine {

inline int curr_state_id = -1;
inline void(*curr_state_exit)() = nullptr;
inline bool(*curr_state_check_transitions)() = nullptr;

template <typename T>
concept StateC = requires {
    { T::ID } -> std::convertible_to<int>;
    { T::on_enter() } -> std::same_as<void>;
    { T::on_exit() } -> std::same_as<void>;
    { T::check_transitions() } -> std::same_as<bool>;
    { T::protocol() } -> std::same_as<Protocol>;
};

template <StateC T>
void enter_state() {
    curr_state_id = T::ID;
    curr_state_exit = T::on_exit;
    curr_state_check_transitions = T::check_transitions;
    T::on_enter();
    T::protocol();
}

inline void leave_state() {
    if (curr_state_exit) {
        curr_state_exit();
        curr_state_exit = nullptr;
    }

    if (Protocol::has_coroutine) {
        Protocol::handle().destroy();
    }
    curr_state_id = -1;
}

template <StateC T>
void change_state() {
    if (curr_state_id == T::ID) return;
    int old_state = curr_state_id;
    leave_state();
    enter_state<T>();
    protocol::on_state_change(old_state, curr_state_id);
}

template <int id>
struct State {
    constexpr static int ID = id;
    static void on_enter() {};
    static void on_exit() {};
    static bool check_transitions() {return false;};
    static Protocol protocol() {co_return;};
};

};
