#pragma once

#include <stdint.h>

// State of charge of a single cell lithium polymer battery.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace BatteryCurve {
  // Voltage the curve was drawn against at the full end. A charger terminates there, but the cell relaxes
  // below it once the current is cut and the load of the device pulls it down further, so what a given board
  // actually reads on a full cell is lower and is passed to percentFromVolts instead.
  constexpr float NOMINAL_FULL_VOLTS = 4.20f;

  // Smallest gap kept between the full anchor and the 90 % point, so the top segment cannot collapse or
  // invert whatever is configured
  constexpr float MIN_FULL_MARGIN = 0.02f;

  // Maps a resting cell voltage to a percentage. The discharge curve is far from linear: half the capacity
  // sits between 3.7 V and 3.9 V, so a linear reading would report 50 % for most of the discharge.
  //
  // fullVolts anchors the 100 % end and only reshapes the segment above 90 %. The rest of the curve is a
  // property of the chemistry and stays where it is, the cut-off included.
  uint8_t percentFromVolts(float volts, float fullVolts = NOMINAL_FULL_VOLTS);

  // Keeps a reported percentage still until the freshly measured one has moved away from it by at least the
  // deadband. Around 3.8 V the curve is steep enough that a single converter step is worth a couple of
  // points, so without this the figure never settles. The ends are always let through, otherwise a full or a
  // flat cell could never be reported as such.
  uint8_t stablePercent(uint8_t reported, uint8_t measured, uint8_t deadband);
}
