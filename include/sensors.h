#pragma once

#include <Arduino.h>

#include "sensors/bmp180.h"
#include "sensors/pms5003.h"
#include "sensors/senseair_s8.h"
#include "sensors/sgp30.h"
#include "sensors/sht3x.h"

// Aggregates every sensor behind a single snapshot, so the display, the web UI and the metrics exporter all
// read the same values.
namespace Sensors {
  struct Snapshot {
    bool hasParticulate = false;
    Pms5003::Data particulate;
    bool fanRunning = false;

    bool hasCo2 = false;
    uint16_t co2Ppm = 0;

    bool hasTvoc = false;
    uint16_t tvocPpb = 0;
    uint16_t eco2Ppm = 0;

    bool hasClimate = false;
    float temperatureC = 0.0f;
    float humidity = 0.0f;

    bool hasPressure = false;
    float pressureHpa = 0.0f;
    float seaLevelPressureHpa = 0.0f;
    float altitudeM = 0.0f;
    float barometerTemperatureC = 0.0f;

    // US EPA air quality index derived from PM2.5, valid only with hasParticulate
    uint16_t aqiUs = 0;
  };

  void begin();
  void loop();

  Snapshot snapshot();
}
