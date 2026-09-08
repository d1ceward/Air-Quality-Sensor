#pragma once

#include <Arduino.h>

// Bosch BMP180 barometric sensor (GY-68 breakout), I2C address 0x77.
//
// Its own temperature reading runs warm because the die self-heats, so the SHT3x stays the reference for
// ambient temperature; the BMP180 temperature is only exposed for diagnostics.
namespace Bmp180 {
  bool begin();
  void loop();

  bool hasReading();
  float pressureHpa();         // measured at the station
  float seaLevelPressureHpa(); // corrected with the configured station altitude
  float altitudeM();           // derived from the standard atmosphere
  float temperatureC();
  uint32_t readingAgeMs();
}
