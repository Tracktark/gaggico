#pragma once
#include "inttypes.hpp"
#include "settings.hpp"

struct OutMessage {
    virtual void write(u8*& ptr) const = 0;
};

struct StateChangeMessage : OutMessage {
    static constexpr i32 ID = 1;
    i32 new_state;

    virtual void write(u8*& ptr) const;
};

struct SensorStatusMessage : OutMessage {
    static constexpr i32 ID = 2;
    float temp;
    float pressure;

    virtual void write(u8*& ptr) const;
};

struct SettingsGetMessage : OutMessage {
    static constexpr i32 ID = 3;

    virtual void write(u8*& ptr) const;
};

struct InMessage {
    virtual void read(u8*& ptr) = 0;
    virtual void handle() = 0;
    virtual ~InMessage() {};
};

struct PowerMessage : InMessage {
    static constexpr i32 ID = 1;
    bool status;

    virtual void read(u8*& ptr);
    virtual void handle();
    virtual ~PowerMessage() {}
};

struct SettingsUpdateMessage : InMessage {
    static constexpr i32 ID = 2;
    Settings settings;

    virtual void read(u8*& ptr);
    virtual void handle();
    virtual ~SettingsUpdateMessage() {}
};

struct GetStatusMessage : InMessage {
    static constexpr i32 ID = 3;

    virtual void read(u8*& ptr);
    virtual void handle();
    virtual ~GetStatusMessage() {}
};
