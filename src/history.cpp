#include <math.h>

#include "history.h"

namespace History {
  namespace {
    constexpr int16_t MAX_SAMPLE = INT16_MAX;
    // NO_SAMPLE takes the bottom of the range, so a real reading stops one above it
    constexpr int16_t MIN_SAMPLE = INT16_MIN + 1;
  }

  void Ring::clear() {
    head_ = 0;
    count_ = 0;
  }

  void Ring::push(const int16_t *values) {
    if (values == nullptr)
      return;

    for (uint8_t series = 0; series < SERIES_COUNT; ++series)
      samples_[head_][series] = values[series];

    head_ = static_cast<uint16_t>((head_ + 1) % CAPACITY);

    if (count_ < CAPACITY)
      ++count_;
  }

  int16_t Ring::at(uint8_t series, uint16_t index) const {
    if (series >= SERIES_COUNT || index >= count_)
      return NO_SAMPLE;

    // The oldest sample sits right after the newest once the window has wrapped
    const uint16_t oldest = static_cast<uint16_t>((head_ + CAPACITY - count_) % CAPACITY);

    return samples_[(oldest + index) % CAPACITY][series];
  }

  uint16_t Ring::read(uint8_t series, int16_t *out, uint16_t outSize) const {
    if (out == nullptr || series >= SERIES_COUNT)
      return 0;

    const uint16_t length = count_ < outSize ? count_ : outSize;

    for (uint16_t index = 0; index < length; ++index)
      out[index] = at(series, index);

    return length;
  }

  int16_t scaled(bool valid, float value, float factor) {
    if (!valid || isnan(value) || isinf(value))
      return NO_SAMPLE;

    const float raw = roundf(value * factor);

    if (raw > static_cast<float>(MAX_SAMPLE))
      return MAX_SAMPLE;

    if (raw < static_cast<float>(MIN_SAMPLE))
      return MIN_SAMPLE;

    return static_cast<int16_t>(raw);
  }

  int16_t clamped(bool valid, uint32_t value) {
    if (!valid)
      return NO_SAMPLE;

    return value > static_cast<uint32_t>(MAX_SAMPLE) ? MAX_SAMPLE : static_cast<int16_t>(value);
  }
}
