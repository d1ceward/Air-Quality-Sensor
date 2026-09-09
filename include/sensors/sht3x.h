#pragma once

#include <Arduino.h>

// Sensirion SHT3x temperature and humidity sensor (I2C).
namespace Sht3x {
  // False when the sensor did not answer. loop() keeps probing it, so this can turn true later.
  bool begin();
  void loop();

  // True once the sensor has answered a probe, whether or not it has produced a reading since.
  bool isAvailable();

  bool hasReading();
  float temperatureC();
  float humidity();
  uint32_t readingAgeMs();
}
