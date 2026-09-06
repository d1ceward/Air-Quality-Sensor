#pragma once

#include <stdint.h>

// State of charge of a single cell lithium polymer battery.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace BatteryCurve {
  // Maps a resting cell voltage to a percentage. The discharge curve is far
  // from linear: half the capacity sits between 3.7 V and 3.9 V, so a linear
  // reading would report 50 % for most of the discharge.
  uint8_t percentFromVolts(float volts);
}
