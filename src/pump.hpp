// Most of the code in this file was shamelessly stolen or adapted from https://github.com/Zer0-bit/gaggiuino
#pragma once
#include <array>
#include <cmath>
#include "config.hpp"

constexpr int maxPumpClicksPerSecond = MAINS_FREQUENCY_HZ;
constexpr int flowPerClickAtZeroBar = -1.f; // TODO: CHANGE THIS
constexpr float fpc_multiplier = 60.f / maxPumpClicksPerSecond;
constexpr std::array pressureInefficiencyCoefficient {
  0.045f,
  0.015f,
  0.0033f,
  0.000685f,
  0.000045f,
  0.009f,
  -0.0018f
};

constexpr float getPumpFlowPerClick(const float pressure) {
  float fpc = 0.f;
  fpc = (pressureInefficiencyCoefficient[5] / pressure + pressureInefficiencyCoefficient[6]) * ( -pressure * pressure ) + ( flowPerClickAtZeroBar - pressureInefficiencyCoefficient[0]) - (pressureInefficiencyCoefficient[1] + (pressureInefficiencyCoefficient[2] - (pressureInefficiencyCoefficient[3] - pressureInefficiencyCoefficient[4] * pressure) * pressure) * pressure) * pressure;
  return fpc * fpc_multiplier;
}

constexpr float getClicksPerSecondForFlow(const float flow, const float pressure) {
  if (flow == 0.f) return 0;
  float flowPerClick = getPumpFlowPerClick(pressure);
  float cps = flow / flowPerClick;
  return fminf(cps, (float)maxPumpClicksPerSecond);
}

constexpr float getPumpPct(float currentPressure, float targetPressure) {
    if (targetPressure < 0.5) {
        return 0;
    }

    float error = targetPressure - currentPressure;
    float maxPumpPct = 1;
        // flowRestriction <= 0.f
        //     ? 1.f
        //     : getClicksPerSecondForFlow(flowRestriction, currentPressure) / (float)maxPumpClicksPerSecond;

    if (error > 2) {
        return fminf(maxPumpPct, 0.25 + 0.2*error);
    }

    // float pumpPctToMaintainFlow =
    //     getClicksPerSecondForFlow(currentFlow, currentPressure) / (float)maxPumpClicksPerSecond;

    if (error > 0) {
        return fminf(maxPumpPct, 0.1 + 0.2 * error);
    }

    // if (pressureChangeSpeed < 0) {
    //     return fminf(maxPumpPct, pumpPctToMaintainFlow * 0.2);
    // }

    return 0;
}
