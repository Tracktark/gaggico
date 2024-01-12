#pragma once
#include "inttypes.hpp"

struct OutMessage {
    virtual void write(u8*& ptr) = 0;
};

struct HelloMessage : OutMessage {
    static const i32 ID = 1;
    i32 num;

    virtual void write(u8*& ptr);
};

struct StatusMessage : OutMessage {
    static const i32 ID = 2;
    float temp;
    float pressure;

    virtual void write(u8*& ptr);
};
