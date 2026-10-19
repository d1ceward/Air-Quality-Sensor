#pragma once

#include <stddef.h>
#include <stdint.h>

// Last few kilobytes of console output, kept in RAM so the web UI can show what the serial port would without
// a cable plugged in. Bytes are addressed by their offset since boot, which lets a client ask for only what it
// has not seen yet and tell a gap or a reboot apart from new output.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace LogRing {
  // Costs as much static RAM. A boot plus a few hours of the sensors' occasional messages fit in it.
  constexpr size_t CAPACITY = 2048;

  class Buffer {
   public:
    void clear();

    void write(uint8_t byte);

    // Bytes written since boot, which is also the offset of the next byte
    uint32_t total() const { return total_; }

    // Offset of the oldest byte still held
    uint32_t oldest() const { return total_ > CAPACITY ? total_ - static_cast<uint32_t>(CAPACITY) : 0; }

    // An offset that fell out of the window starts at the oldest byte still held. So does one past the end,
    // which is what a client that last polled before a reboot sends.
    uint32_t clamp(uint32_t offset) const;

    // Copies the bytes from offset on, after clamp(), into out and returns how many were written, capped by
    // outSize.
    size_t read(uint32_t offset, char *out, size_t outSize) const;

   private:
    char data_[CAPACITY] = {};
    uint32_t total_ = 0;
  };
}
