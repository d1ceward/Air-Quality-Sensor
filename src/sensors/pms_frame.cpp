#include "sensors/pms_frame.h"

namespace {
  constexpr uint8_t FRAME_START_1 = 0x42;
  constexpr uint8_t FRAME_START_2 = 0x4D;
}

uint16_t PmsFrameParser::word16(uint8_t high, uint8_t low) {
  return static_cast<uint16_t>((high << 8) | low);
}

void PmsFrameParser::reset() {
  index = 0;
  frameLength = 0;
  checksum = 0;
}

void PmsFrameParser::decode() {
  // Payload bytes 0 to 5 hold the CF=1 standard particle values, which are calibrated for an industrial
  // environment and are not what we report.
  lastFrame.pm1 = word16(payload[6], payload[7]);
  lastFrame.pm25 = word16(payload[8], payload[9]);
  lastFrame.pm10 = word16(payload[10], payload[11]);
  lastFrame.count03 = word16(payload[12], payload[13]);
  lastFrame.count05 = word16(payload[14], payload[15]);
  lastFrame.count10 = word16(payload[16], payload[17]);
  lastFrame.count25 = word16(payload[18], payload[19]);
  lastFrame.count50 = word16(payload[20], payload[21]);
  lastFrame.count100 = word16(payload[22], payload[23]);
}

bool PmsFrameParser::feed(uint8_t value) {
  switch (index) {
    case 0:
      if (value != FRAME_START_1)
        return false;

      checksum = value;
      break;

    case 1:
      if (value != FRAME_START_2) {
        // The byte may itself start a frame, so restart rather than drop it
        index = 0;
        return feed(value);
      }

      checksum += value;
      break;

    case 2:
      checksum += value;
      frameLength = static_cast<uint16_t>(value << 8);
      break;

    case 3:
      frameLength |= value;
      // A PMS5003 announces 28 bytes; anything else is a transmission error
      if (frameLength != PAYLOAD_SIZE) {
        index = 0;
        return false;
      }

      checksum += value;
      break;

    default: {
      const uint8_t payloadIndex = index - 4;
      payload[payloadIndex] = value;

      // The last two payload bytes carry the checksum itself
      if (payloadIndex < PAYLOAD_SIZE - 2) {
        checksum += value;
        break;
      }

      if (payloadIndex == PAYLOAD_SIZE - 1) {
        index = 0;

        if (checksum != word16(payload[PAYLOAD_SIZE - 2], payload[PAYLOAD_SIZE - 1]))
          return false;

        decode();
        return true;
      }

      break;
    }
  }

  ++index;
  return false;
}
