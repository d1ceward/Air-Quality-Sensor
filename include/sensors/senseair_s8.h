#pragma once

#include <Arduino.h>

// SenseAir S8 CO2 sensor, read over Modbus RTU (input register 3).
namespace SenseAirS8 {
  void begin();
  void loop();

  bool hasReading();
  uint16_t co2Ppm();
  uint32_t readingAgeMs();

  // Triggers a manual background calibration. The sensor must have been in
  // fresh outdoor air (~400 ppm) for a few minutes beforehand.
  bool calibrate();
}
