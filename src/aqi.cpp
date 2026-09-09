#include "aqi.h"

namespace Aqi {
  namespace {
    // US EPA breakpoints: concentration range then the matching index range
    struct Breakpoint {
      float concentrationLow;
      float concentrationHigh;
      float indexLow;
      float indexHigh;
    };

    // Table revised by the EPA in February 2024 (effective 6 May 2024) alongside the annual PM2.5 standard
    // that dropped from 12.0 to 9.0 ug/m3. The old table reported 42 where this one reports 53 at
    // 10 ug/m3, so a dashboard built on the previous firmware shows a step at the upgrade.
    const Breakpoint BREAKPOINTS[] = {
      {0.0f, 9.0f, 0.0f, 50.0f},
      {9.1f, 35.4f, 51.0f, 100.0f},
      {35.5f, 55.4f, 101.0f, 150.0f},
      {55.5f, 125.4f, 151.0f, 200.0f},
      {125.5f, 225.4f, 201.0f, 300.0f},
      // The EPA splits the hazardous band again above 325.4 for AQI 501 and beyond; the scale published
      // here stops at 500.
      {225.5f, 325.4f, 301.0f, 500.0f},
    };
  }

  uint16_t fromPm25Us(uint16_t pm25) {
    const float concentration = static_cast<float>(pm25);

    for (const Breakpoint &breakpoint : BREAKPOINTS) {
      if (concentration > breakpoint.concentrationHigh)
        continue;

      const float span = breakpoint.concentrationHigh - breakpoint.concentrationLow;
      const float indexSpan = breakpoint.indexHigh - breakpoint.indexLow;
      const float offset = concentration - breakpoint.concentrationLow;

      return static_cast<uint16_t>(indexSpan / span * offset + breakpoint.indexLow + 0.5f);
    }

    return 500;
  }
}
