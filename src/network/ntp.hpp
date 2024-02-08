#pragma once
#include <pico/time.h>
#include "inttypes.hpp"

namespace ntp {
void init();
u64 get_time_us();
u64 to_timestamp(absolute_time_t time);
}
