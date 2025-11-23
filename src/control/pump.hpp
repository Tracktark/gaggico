#pragma once
#include "control/control.hpp"
#include "config.hpp"
#include "settings.hpp"
#include <cmath>

// Coeffitients for a polynomial, which maps a pressure to the max
// flow of the pump per click. The polynomial was fitted based on values from
// https://cdn.shopify.com/s/files/1/0503/8833/6813/files/E_high_pressure.pdf
// NOTE: The values in the datasheet are measured in ml/min, however
// the datasheet doesn't specify if those values apply for 50 or 60 hz,
// so during conversion to ml/click I assumed they were meant for 50 hz.
constexpr float pressure_to_flow_coeff[] = {
    -4.6822412444256283e-07,
    2.2158122761297405e-05,
    -0.00043141565482301947,
    0.004065919229544844,
    -0.029405108329017122,
    0.2171117797405085,
};

constexpr float get_flow_per_click_datasheet(float pressure) {
    float result = pressure_to_flow_coeff[0];
    for (size_t i = 1; i < (sizeof(pressure_to_flow_coeff) / sizeof(float)); ++i) {
        result *= pressure;
        result += pressure_to_flow_coeff[i];
    }
    return result;
}
constexpr float get_flow_per_click(float pressure, float pump_zero) {
    return get_flow_per_click_datasheet(pressure)
        + pump_zero;
}
inline float get_flow(float pressure, float click_count) {
    return get_flow_per_click(pressure, settings::get().pump_zero) * click_count;
}

constexpr float get_power_for_pressure(float target_pressure, float current_pressure) {
    float error = target_pressure - current_pressure;
    if (error <= 0)
        return 0;

    constexpr auto FULL_AT = 3;
    constexpr auto STEEPNESS = 0.4;
    return powf(error / FULL_AT, STEEPNESS);
}
inline float get_power_for_flow(float target_flow, float current_pressure) {
    if (target_flow <= 0.0f) return 0;
    float flow_per_click = get_flow_per_click(current_pressure, settings::get().pump_zero);
    float clicks_per_second = target_flow / flow_per_click;
    float power = clicks_per_second / MAINS_FREQUENCY_HZ;
    return power;
}

inline float calculate_desired_power(const control::Sensors& sensors,
                                     float target_pressure,
                                     float target_flow) {
    float power_pressure = get_power_for_pressure(target_pressure, sensors.pressure);
    float power_flow = get_power_for_flow(target_flow, sensors.pressure);
    return fminf(fminf(power_pressure, power_flow), 1.f);
}
