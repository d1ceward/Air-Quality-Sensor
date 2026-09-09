#pragma once

#include <Arduino.h>

// Persists the SGP30 calibration baseline in its own small file.
//
// It used to sit in the configuration, which meant the hourly baseline refresh rewrote the WiFi credentials
// and the admin password along with it. Its own file keeps that hourly write away from anything that would
// cost a trip to the device to recover.
namespace BaselineStore {
  struct Baseline {
    uint16_t eco2 = 0;
    uint16_t tvoc = 0;
    uint32_t epoch = 0;  // when it was captured, zero when the clock was not synced yet
  };

  // Reads the stored baseline. Falls back once to the copy an older firmware left in the configuration, and
  // returns false when neither holds anything usable.
  bool load(Baseline &result);

  bool save(const Baseline &value);
}
