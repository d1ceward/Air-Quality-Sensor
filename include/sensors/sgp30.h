#pragma once

#include <Arduino.h>

// Sensirion SGP30 volatile organic compound sensor (I2C).
namespace Sgp30 {
  // False when the sensor did not answer. loop() keeps probing it, so this can turn true later.
  bool begin();
  void loop();

  // True once the sensor has answered a probe, whether or not it has produced a reading since.
  bool isAvailable();

  bool hasReading();
  uint16_t tvocPpb();
  uint16_t eco2Ppm();
  uint32_t readingAgeMs();

  // Feeds ambient conditions to the on-chip humidity compensation.
  void setHumidityCompensation(float temperatureC, float relativeHumidity);
}
