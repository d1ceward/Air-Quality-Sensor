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

    const Breakpoint BREAKPOINTS[] = {
      {0.0f, 12.0f, 0.0f, 50.0f},
      {12.1f, 35.4f, 51.0f, 100.0f},
      {35.5f, 55.4f, 101.0f, 150.0f},
      {55.5f, 150.4f, 151.0f, 200.0f},
      {150.5f, 250.4f, 201.0f, 300.0f},
      {250.5f, 350.4f, 301.0f, 400.0f},
      {350.5f, 500.4f, 401.0f, 500.0f},
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
