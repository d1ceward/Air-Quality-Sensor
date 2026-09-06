#include <math.h>

#include "barometry.h"

namespace Barometry {
  namespace {
    constexpr float EXPONENT = 5.255f;
    constexpr float SCALE_M = 44330.0f;
  }

  float toSeaLevelHpa(float measuredHpa, float stationAltitudeMeters) {
    if (stationAltitudeMeters == 0.0f)
      return measuredHpa;

    return measuredHpa / powf(1.0f - stationAltitudeMeters / SCALE_M, EXPONENT);
  }

  float toAltitudeMeters(float measuredHpa) {
    if (measuredHpa <= 0.0f)
      return 0.0f;

    return SCALE_M * (1.0f - powf(measuredHpa / SEA_LEVEL_PRESSURE_HPA, 1.0f / EXPONENT));
  }
}
