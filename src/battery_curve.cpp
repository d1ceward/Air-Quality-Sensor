#include "battery_curve.h"

namespace BatteryCurve {
  namespace {
    struct Point {
      float volts;
      uint8_t percent;
    };

    // Typical single cell LiPo discharge curve, from full to the cut-off. The head voltage is nominal: the
    // caller replaces it with what its own board reads on a full cell.
    const Point CURVE[] = {
      {NOMINAL_FULL_VOLTS, 100}, {4.10f, 90}, {4.00f, 80}, {3.95f, 70}, {3.87f, 60},
      {3.82f, 50}, {3.79f, 40}, {3.77f, 30}, {3.74f, 20}, {3.68f, 10},
      {3.45f, 5}, {3.00f, 0},
    };

    constexpr uint8_t POINT_COUNT = sizeof(CURVE) / sizeof(CURVE[0]);
  }

  uint8_t percentFromVolts(float volts, float fullVolts) {
    // An anchor at or below the 90 % point would give the top segment a zero or negative span
    const float floorVolts = CURVE[1].volts + MIN_FULL_MARGIN;
    const float topVolts = fullVolts > floorVolts ? fullVolts : floorVolts;

    if (volts >= topVolts)
      return 100;

    if (volts <= CURVE[POINT_COUNT - 1].volts)
      return 0;

    for (uint8_t i = 1; i < POINT_COUNT; ++i) {
      if (volts > CURVE[i].volts) {
        const Point &upper = CURVE[i - 1];
        const Point &lower = CURVE[i];
        // Only the head of the curve moves with the anchor, the points below it are fixed
        const float upperVolts = i == 1 ? topVolts : upper.volts;
        const float span = upperVolts - lower.volts;
        const float offset = volts - lower.volts;
        const float percentSpan = static_cast<float>(upper.percent - lower.percent);

        return static_cast<uint8_t>(lower.percent + percentSpan * offset / span + 0.5f);
      }
    }

    return 0;
  }

  uint8_t stablePercent(uint8_t reported, uint8_t measured, uint8_t deadband) {
    if (measured == 0 || measured == 100)
      return measured;

    const int16_t drift = static_cast<int16_t>(measured) - static_cast<int16_t>(reported);

    if (drift >= deadband || -drift >= deadband)
      return measured;

    return reported;
  }
}
