#include "ntp.hpp"
#include <ctime>
#include <hardware/rtc.h>
#include <hardware/timer.h>
#include <hardware/divider.h>
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


    auto res = hw_divider_divmod_u32(sec, 60*60*24);
    u32 days = to_quotient_u32(res);
    u32 sod = to_remainder_u32(res);
    days += 719468;
    res = hw_divider_divmod_u32(days, 146097);
    u32 era = to_quotient_u32(res);
    u32 doe = to_remainder_u32(res);
    u32 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    u32 year = yoe + era * 400;
    u32 doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    u32 mp = (5 * doy + 2) / 153;
    u32 day = doy - (153 * mp + 2) / 5 + 1;
    u32 month = mp + (mp < 10 ? 3 : -9);

    res = hw_divider_divmod_u32(sod, 60 * 60);
    u32 hour = to_quotient_u32(res);
    u32 soh = to_remainder_u32(res);
    res = hw_divider_divmod_u32(soh, 60);
    u32 minute = to_quotient_u32(res);
    u32 second = to_remainder_u32(res);

    datetime_t datetime {
        .year = static_cast<i16>(year),
        .month = static_cast<i8>(month),
        .day = static_cast<i8>(day),
        .dotw = 0, // We don't care about day of the week
        .hour = static_cast<i8>(hour),
        .min = static_cast<i8>(minute),
        .sec = static_cast<i8>(second),
    };
    rtc_set_datetime(&datetime);
}
}

void ntp::init() {
    // Assume it is 1.1.CURRENT_YEAR at 0:00
    constexpr u64 curr_epoch_us = (CURRENT_YEAR - 1970) * 365.2425 * 24 * 60 * 60 * 1'000'000;
    timer_to_epoch_offset_us = curr_epoch_us - time_us_64();

    rtc_init();
    sntp_init();
}

u64 ntp::get_time_us() {
    return ntp_get_system_time_us();
}

u64 ntp::to_timestamp(absolute_time_t time) {
    return to_us_since_boot(time) + timer_to_epoch_offset_us;
}
