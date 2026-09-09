#pragma once

#include <stdint.h>

// Rolling window of past readings, kept in RAM so the web UI can draw a trend next to each value. Nothing is
// persisted: the point is to see where a reading is heading over the last couple of hours, not to replace the
// Prometheus server that keeps the long history.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace History {
  // Order matters, it is the order the samples are pushed and read back in
  enum Series : uint8_t {
    SERIES_PM25 = 0,       // ug/m3
    SERIES_CO2,            // ppm
    SERIES_TVOC,           // ppb
    SERIES_TEMPERATURE,    // degrees Celsius times ten
    SERIES_HUMIDITY,       // percent times ten
    SERIES_PRESSURE,       // hPa times ten
    SERIES_BATTERY,        // percent
    SERIES_COUNT
  };

  // Two hours at one sample per minute. Every slot costs SERIES_COUNT * 2 bytes of static RAM.
  constexpr uint16_t CAPACITY = 120;

  // Marks a slot where the sensor had nothing valid to give. Sits outside every scaled range above.
  constexpr int16_t NO_SAMPLE = INT16_MIN;

  class Ring {
   public:
    void clear();

    // Appends one sample per series, oldest dropped once the window is full. values must hold SERIES_COUNT
    // entries; use NO_SAMPLE for a series with no valid reading.
    void push(const int16_t *values);

    uint16_t size() const { return count_; }
    bool isFull() const { return count_ == CAPACITY; }

    // Copies one series oldest first into out and returns how many entries were written, capped by outSize.
    uint16_t read(uint8_t series, int16_t *out, uint16_t outSize) const;

    // Single sample of a series, index 0 being the oldest still held.
    int16_t at(uint8_t series, uint16_t index) const;

   private:
    int16_t samples_[CAPACITY][SERIES_COUNT] = {};
    uint16_t head_ = 0;   // next slot to write
    uint16_t count_ = 0;
  };

  // Rounds a float to a scaled integer, clamped to the storable range. Returns NO_SAMPLE when the reading is
  // not valid, so a gap in the window stays a gap rather than a zero the sparkline would draw through.
  int16_t scaled(bool valid, float value, float factor);

  // Same for a reading that is already an integer, clamped rather than wrapped: the SGP30 reports TVOC up to
  // 60000 ppb, well past what a slot holds.
  int16_t clamped(bool valid, uint32_t value);
}
