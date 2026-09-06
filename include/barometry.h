#pragma once

// Barometric conversions against the International Standard Atmosphere.
//
// Free of Arduino dependencies so they can be exercised on the host.
namespace Barometry {
  constexpr float SEA_LEVEL_PRESSURE_HPA = 1013.25f;

  // Reduces a station reading to sea level, the figure weather forecasts use.
  float toSeaLevelHpa(float measuredHpa, float stationAltitudeMeters);

  // Altitude the standard atmosphere would put this pressure at. It moves with
  // the weather, so it indicates rather than locates.
  float toAltitudeMeters(float measuredHpa);
}
