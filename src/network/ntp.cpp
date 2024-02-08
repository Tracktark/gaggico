#include "ntp.hpp"
#include <hardware/timer.h>
#include <lwip/apps/sntp.h>
#include <cstdio>

static u64 timer_to_epoch_offset_us = 0;

extern "C" {
u64 ntp_get_system_time_us() {
    u64 curr_epoch = time_us_64() + timer_to_epoch_offset_us;
    return curr_epoch;
}

void ntp_set_system_time(u32 sec, u32 us) {
    u64 curr_epoch_us = static_cast<u64>(sec) * 1'000'000LL + static_cast<u64>(us);
    timer_to_epoch_offset_us = curr_epoch_us - time_us_64();
}
}

void ntp::init() {
    // Assume it is 1.1.CURRENT_YEAR at 0:00
    constexpr u64 curr_epoch_us = (CURRENT_YEAR - 1970) * 365.2425 * 24 * 60 * 60 * 1'000'000;
    timer_to_epoch_offset_us = curr_epoch_us - time_us_64();

    sntp_init();
}

u64 ntp::get_time_us() {
    return ntp_get_system_time_us();
}

u64 ntp::to_timestamp(absolute_time_t time) {
    return to_us_since_boot(time) + timer_to_epoch_offset_us;
}
