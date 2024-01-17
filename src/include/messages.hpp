#pragma once
#include "inttypes.hpp"

struct OutMessage {
    virtual void write(u8*& ptr) = 0;
};

struct StateChangeMessage : OutMessage {
    static constexpr i32 ID = 1;
    i32 new_state;

    virtual void write(u8*& ptr);
};

struct SensorStatusMessage : OutMessage {
    static constexpr i32 ID = 2;
    float temp;
    float pressure;

    virtual void write(u8*& ptr);
};
struct InMessage {
    virtual void read(u8*& ptr) = 0;
    virtual void handle() = 0;
    virtual ~InMessage() {};
};

