#pragma once

#include <Arduino.h>

// Bosch BMP180 barometric sensor (GY-68 breakout), I2C address 0x77.
//
// Its own temperature reading runs warm because the die self-heats, so the SHT3x stays the reference for
// ambient temperature; the BMP180 temperature is only exposed for diagnostics.
namespace Bmp180 {
  // False when the sensor did not answer. loop() keeps probing it, so this can turn true later.
  bool begin();
  void loop();

  // True once the sensor has answered a probe, whether or not it has produced a reading since.
  bool isAvailable();

  bool hasReading();
  float pressureHpa();         // measured at the station
  float seaLevelPressureHpa(); // corrected with the configured station altitude
  float altitudeM();           // derived from the standard atmosphere
  float temperatureC();
  uint32_t readingAgeMs();
}
