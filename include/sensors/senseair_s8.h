#pragma once

#include <Arduino.h>

// SenseAir S8 CO2 sensor, read over Modbus RTU (input register 3).
namespace SenseAirS8 {
  void begin();
  void loop();

  // True once the sensor has answered a request. There is nothing to probe on a UART, so a sensor that has
  // never replied is indistinguishable from one that is not wired at all.
  bool isAvailable();

  bool hasReading();
  uint16_t co2Ppm();
  uint32_t readingAgeMs();

  // Triggers a manual background calibration. The sensor must have been in fresh outdoor air (~400 ppm) for a
  // few minutes beforehand.
  bool calibrate();
}
