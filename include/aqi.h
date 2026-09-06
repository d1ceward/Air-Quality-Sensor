#pragma once

#include <stdint.h>

// US EPA air quality index.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace Aqi {
  // Converts a PM2.5 concentration in ug/m3, clamped to 500 above the last
  // breakpoint.
  uint16_t fromPm25Us(uint16_t pm25);
}
