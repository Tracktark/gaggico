#pragma once
#include "messages.hpp"

namespace network {
void wifi_init();
void server_init();
void process_outgoing_messages();
usize message_queue_size();
void enqueue_message(const OutMessages& msg);
}
