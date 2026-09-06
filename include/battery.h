#pragma once

#include <Arduino.h>

// Battery monitoring for the board fitted with a Wemos D1 mini battery shield.
//
// The shield's 130 kOhm resistor sits in series with the 220k/100k divider of
// the D1 mini, so 450 kOhm total with the ADC reading across the lower 100k:
// the 1 V full scale of the converter lands at 4.5 V on the cell.
namespace Battery {
  void begin();
  void loop();

  // False when no battery is configured, which is the case by default
  bool isEnabled();
  bool hasReading();

  float volts();
  uint8_t percent();
}
