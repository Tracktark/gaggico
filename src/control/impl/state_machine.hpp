#pragma once

#include <concepts>
#include <tuple>
#include "coroutine.hpp"
#include "control/protocol.hpp"

namespace statemachine {

volatile inline int curr_state_id = -1;
inline void(*curr_state_exit)() = nullptr;
inline bool(*curr_state_check_transitions)() = nullptr;

template <typename T>
concept StateC = requires {
    { T::ID } -> std::convertible_to<int>;
    { T::on_enter() } -> std::same_as<void>;
    { T::on_exit() } -> std::same_as<void>;
    { T::check_transitions() } -> std::same_as<bool>;
    { T::coroutine() } -> std::same_as<Coroutine>;
};

template <StateC T>
void enter_state() {
    curr_state_id = T::ID;
    curr_state_exit = T::on_exit;
    curr_state_check_transitions = T::check_transitions;
    T::on_enter();
    T::coroutine();
}

inline void leave_state() {
    if (curr_state_exit) {
        curr_state_exit();
        curr_state_exit = nullptr;
    }

    if (Coroutine::has_coroutine) {
        Coroutine::handle().destroy();
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
    static Coroutine coroutine() {co_return;};
};

template <typename States>
struct change_state_by_id_impl;

template <StateC T, StateC... Rest>
struct change_state_by_id_impl<std::tuple<T, Rest...>> {
    static void exec(int id) {
        if (T::ID == id) {
            change_state<T>();
        } else {
            change_state_by_id_impl<std::tuple<Rest...>>::exec(id);
        }
    }
};

template <>
struct change_state_by_id_impl<std::tuple<>> {
    static void exec(int id) { (void)id; }
};

template <typename States>
void change_state_by_id(int id) {
    change_state_by_id_impl<States>::exec(id);
}
};
