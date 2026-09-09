#include "uptime.h"

namespace Uptime {
  namespace {
    uint32_t lastTicks = 0;
    uint32_t wraps = 0;
    bool started = false;

    uint64_t extended() {
      return (static_cast<uint64_t>(wraps) << 32) | lastTicks;
    }
  }

  uint64_t tick(uint32_t ticks) {
    // A counter that went backwards can only have wrapped: nothing else moves it down
    if (started && ticks < lastTicks)
      ++wraps;

    started = true;
    lastTicks = ticks;

    return extended();
  }

  uint32_t seconds() {
    return static_cast<uint32_t>(extended() / 1000ULL);
  }

  uint64_t millis64() {
    return extended();
  }

  void reset() {
    lastTicks = 0;
    wraps = 0;
    started = false;
  }
}
