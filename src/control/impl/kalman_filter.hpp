/*
 * SimpleKalmanFilter - a Kalman Filter implementation for single variable models.
 * Created by Denys Sene, January, 1, 2017.
 * https://github.com/denyssene/SimpleKalmanFilter
 */
#pragma once

#include <cmath>
class SimpleKalmanFilter {
    float err_measure;
    float err_estimate;
    float init_err_estimate;
    float q;
    float last_estimate = 0;

public:
    SimpleKalmanFilter(float err_measure, float err_estimate, float q):
        err_measure(err_measure),
        err_estimate(err_estimate),
        init_err_estimate(err_estimate),
        q(q) {}

    float update(float value) {
        float kalman_gain = err_estimate / (err_estimate + err_measure);
        float current_estimate = last_estimate + kalman_gain*(value - last_estimate);
        err_estimate = (1 - kalman_gain)*err_estimate + std::abs(last_estimate - current_estimate)*q;
        last_estimate = current_estimate;

        return current_estimate;
    }

    void reset(float value) {
        last_estimate = value;
        err_estimate = init_err_estimate;
    }
};
