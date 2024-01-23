#pragma once
template <int N>
class MovingAverage {
    float samples[N] = {0};
    int index = 0;
    float sum = 0;

public:
    constexpr void push(float sample) {
        sum += sample - samples[index];

        samples[index] = sample;
        index = (index+1) % N;
    }

    constexpr float average() const {
        return sum / N;
    }
};
