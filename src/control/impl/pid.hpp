#pragma once
#include <algorithm>
#include <pico/time.h>

class PID {
    float kP, kI, kD;
    float last_value = 0;
    float accumulator = 0;

    float outMin = 0, outMax = 0;

    float target_value = 0;
    absolute_time_t last_update_time {0};

public:
    constexpr explicit PID(float kP, float kI, float kD, float outMin, float outMax)
        : kP(kP), kI(kI), kD(kD), outMin(outMin), outMax(outMax) {}

    constexpr void set_target(float target) {
        target_value = target;
    }
    constexpr float get_target() const {
        return target_value;
    }

    float update(float curr_value) {
        absolute_time_t curr_time = get_absolute_time();
        float delta_time = absolute_time_diff_us(last_update_time, curr_time) / 1'000'000.0;
        last_update_time = curr_time;

        float error = target_value - curr_value;

        float change_rate = -(curr_value - last_value) / delta_time;
        last_value = curr_value;

        accumulator += kI * error * delta_time;

        accumulator = std::clamp(accumulator, outMin, outMax);

        return std::clamp(error * kP + change_rate * kD + accumulator, outMin, outMax);
    }

    void reset(float curr_value) {
        last_value = curr_value;
        last_update_time = get_absolute_time();
        accumulator = 0;
    }

    void set_params(float kp, float ki, float kd) {
        this->kP = kp;
        this->kI = ki;
        this->kD = kd;
    }
};
