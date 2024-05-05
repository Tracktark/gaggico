#pragma once
#include "impl/message.hpp"
#include "inttypes.hpp"

namespace network {

constexpr auto BACKLOG = 5;
constexpr auto CLIENT_CAPACITY = 10;

extern std::array<u8, 127> out_message_buffer;

void wifi_init();
void server_init();
void send_buffer(u32 length);

template <OutgoingMsg Msg>
void send(const Msg& msg) {
    u8* start = out_message_buffer.data() + sizeof(u32);
    u8* end = start;

    write_val(end, Msg::OUTGOING_ID);
    write_struct(msg, end);

    send_buffer(end-start);
}
}
