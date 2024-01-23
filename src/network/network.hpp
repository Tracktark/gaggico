#pragma once
#include "inttypes.hpp"
#include "messages.hpp"

namespace network {

constexpr auto BACKLOG = 5;
constexpr auto CLIENT_CAPACITY = 10;

void wifi_init();
void server_init();

void send(const OutMessage& msg);
}
