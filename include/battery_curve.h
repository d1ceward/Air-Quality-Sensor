#pragma once

#include <stdint.h>

// State of charge of a single cell lithium polymer battery.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace BatteryCurve {
  // Maps a resting cell voltage to a percentage. The discharge curve is far from linear: half the capacity
  // sits between 3.7 V and 3.9 V, so a linear reading would report 50 % for most of the discharge.
  uint8_t percentFromVolts(float volts);

  // Keeps a reported percentage still until the freshly measured one has moved away from it by at least the
  // deadband. Around 3.8 V the curve is steep enough that a single converter step is worth a couple of
  // points, so without this the figure never settles. The ends are always let through, otherwise a full or a
  // flat cell could never be reported as such.
  uint8_t stablePercent(uint8_t reported, uint8_t measured, uint8_t deadband);
}
