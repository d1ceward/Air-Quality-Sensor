#pragma once

#include <Arduino.h>

// Plantower PMS5003 particulate matter sensor.
//
// The sensor streams a 32 byte frame roughly once per second while its fan is running. To extend the fan
// lifetime it is duty cycled: wake up, warm up, average a few frames, then sleep.
namespace Pms5003 {
  struct Data {
    uint16_t pm1 = 0;    // ug/m3, atmospheric environment
    uint16_t pm25 = 0;
    uint16_t pm10 = 0;
    uint16_t count03 = 0;  // particles > 0.3 um per 0.1 L
    uint16_t count05 = 0;
    uint16_t count10 = 0;
    uint16_t count25 = 0;
    uint16_t count50 = 0;
    uint16_t count100 = 0;
  };

  void begin();
  void loop();

  bool hasReading();
  const Data &data();
  uint32_t readingAgeMs();
  bool isFanRunning();
}
