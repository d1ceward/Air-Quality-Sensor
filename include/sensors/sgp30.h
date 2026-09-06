#pragma once

#include <Arduino.h>

// Sensirion SGP30 volatile organic compound sensor (I2C).
namespace Sgp30 {
  bool begin();
  void loop();

  bool hasReading();
  uint16_t tvocPpb();
  uint16_t eco2Ppm();
  uint32_t readingAgeMs();

  // Feeds ambient conditions to the on-chip humidity compensation.
  void setHumidityCompensation(float temperatureC, float relativeHumidity);
}
