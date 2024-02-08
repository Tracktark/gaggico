#pragma once
#include "control/protocol.hpp"
#include "inttypes.hpp"
#include "ntp.hpp"
#include "serde.hpp"
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

struct InMessage {
    virtual void read(u8*& ptr) = 0;
    virtual void handle() = 0;
    virtual ~InMessage() {};
};

struct PowerMessage : InMessage {
    static constexpr i32 ID = 1;
    bool status;

    void read(u8*& ptr) override {
        read_val(ptr, status);
    }

    void handle() override {
        protocol::set_power(status);
    }
};

struct SettingsUpdateMessage : InMessage {
    static constexpr i32 ID = 2;
    Settings settings;

    void read(u8*& ptr) override {
        settings.read(ptr);
    }
    void handle() override {
        settings::update(settings);
    }
};

struct GetStatusMessage : InMessage {
    static constexpr i32 ID = 3;

    void read(u8*& ptr) override {}
    void handle() override;
};
