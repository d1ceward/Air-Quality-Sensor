#pragma once

#include <stddef.h>
#include <stdint.h>

// Parser for the 32 byte frames a Plantower PMS5003 streams once per second.
//
// Bytes are fed one at a time so the caller never blocks, the parser resynchro-
// nises on the 0x42 0x4D header and a frame is only reported once its checksum
// matches. Free of Arduino dependencies so the protocol can be exercised on the
// host.
class PmsFrameParser {
public:
  struct Frame {
    // Atmospheric environment concentrations, in ug/m3
    uint16_t pm1 = 0;
    uint16_t pm25 = 0;
    uint16_t pm10 = 0;
    // Particles above the given size, per 0.1 L of air
    uint16_t count03 = 0;
    uint16_t count05 = 0;
    uint16_t count10 = 0;
    uint16_t count25 = 0;
    uint16_t count50 = 0;
    uint16_t count100 = 0;
  };

  // Returns true when this byte completes a valid frame
  bool feed(uint8_t value);

  // Drops any partially received frame
  void reset();

  const Frame &frame() const { return lastFrame; }

private:
  // 13 data words plus the checksum word
  static constexpr uint8_t PAYLOAD_SIZE = 28;

  static uint16_t word16(uint8_t high, uint8_t low);
  void decode();

  uint8_t payload[PAYLOAD_SIZE] = {0};
  uint8_t index = 0;
  uint16_t frameLength = 0;
  uint16_t checksum = 0;
  Frame lastFrame;
};
