#pragma once

#include <stdint.h>

// Night window for the display brightness.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace DisplaySchedule {
  // True when hour falls inside [startHour, endHour), a window that usually
  // wraps around midnight. An empty window, both bounds equal, means the night
  // mode is off.
  bool isNight(uint8_t hour, uint8_t startHour, uint8_t endHour);
}
