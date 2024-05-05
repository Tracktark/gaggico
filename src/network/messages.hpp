#pragma once
#include <variant>

#include "control/protocol.hpp"
#include "inttypes.hpp"
#include "settings.hpp"

struct StateChangeMessage {
    static constexpr i32 OUTGOING_ID = 1;
    u64 state_change_timestamp;
    u64 machine_start_timestamp;
    i32 new_state;
};

struct SensorStatusMessage {
    static constexpr i32 OUTGOING_ID = 2;
    float temp;
    float pressure;
};

struct SettingsGetMessage {
    static constexpr i32 OUTGOING_ID = 3;

    void write(u8*& ptr) const {
        settings::get().write(ptr);
    }
};

struct PowerMessage {
    static constexpr i32 INCOMING_ID = 1;
    bool status;

    void handle() {
        protocol::set_power(status);
    }
};

struct SettingsUpdateMessage {
    static constexpr i32 INCOMING_ID = 2;
    Settings settings;

    void read(u8*& ptr) {
        settings.read(ptr);
    }
    void handle() {
        settings::update(settings);
    }
};

struct GetStatusMessage  {
    static constexpr i32 INCOMING_ID = 3;

    void handle();
};

using InMessages = std::variant<PowerMessage, SettingsUpdateMessage, GetStatusMessage>;
