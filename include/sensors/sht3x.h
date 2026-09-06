#pragma once

#include <Arduino.h>

// Sensirion SHT3x temperature and humidity sensor (I2C).
namespace Sht3x {
  bool begin();
  void loop();

  bool hasReading();
  float temperatureC();
  float humidity();
  uint32_t readingAgeMs();
}
