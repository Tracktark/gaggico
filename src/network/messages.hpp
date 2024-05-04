#pragma once
#include <variant>

#include "control/protocol.hpp"
#include "inttypes.hpp"
#include "impl/serde.hpp"
#include "settings.hpp"

struct OutMessage {
    virtual void write(u8*& ptr) const = 0;
};

struct StateChangeMessage : OutMessage {
    static constexpr i32 ID = 1;
    u64 state_change_timestamp;
    u64 machine_start_timestamp;
    i32 new_state;

    void write(u8*& ptr) const override {
        write_val(ptr, ID);
        write_val(ptr, state_change_timestamp);
        write_val(ptr, machine_start_timestamp);
        write_val(ptr, new_state);
    }
};

struct SensorStatusMessage : OutMessage {
    static constexpr i32 ID = 2;
    float temp;
    float pressure;

    void write(u8*& ptr) const override  {
        write_val(ptr, ID);
        write_val(ptr, temp);
        write_val(ptr, pressure);
    }
};

struct SettingsGetMessage : OutMessage {
    static constexpr i32 ID = 3;

    void write(u8*& ptr) const override {
        write_val(ptr, ID);
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
