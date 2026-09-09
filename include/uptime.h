#pragma once

#include <stdint.h>

// millis() counts in 32 bits, so it wraps back to zero after 49.71 days and an uptime derived straight from
// it drops to zero with the board still running. Feeding the raw counter through here counts the wraps and
// rebuilds a monotonic 64 bit millisecond count.
//
// The wrap is only detected by seeing the counter go backwards, so tick() has to be called at least once per
// wrap; loop() calls it on every iteration.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace Uptime {
  // Records a raw counter value and returns the extended count.
  uint64_t tick(uint32_t ticks);

  // Extended count in whole seconds, without recording a new value.
  uint32_t seconds();

  // Extended count in milliseconds, without recording a new value.
  uint64_t millis64();

  // Drops the accumulated wraps, for the tests.
  void reset();
}
