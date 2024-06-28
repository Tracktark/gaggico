#pragma once
#include "messages.hpp"

namespace network {

constexpr auto CLIENT_CAPACITY = 5;

void wifi_init();
void server_init();
void process_outgoing_messages();
void enqueue_message(const OutMessages& msg);
}
