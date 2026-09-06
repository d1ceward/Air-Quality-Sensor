#include <SoftwareSerial.h>

// Local includes
#include "config.h"
#include "sensors/pms5003.h"

namespace Pms5003 {
  namespace {
    constexpr uint8_t FRAME_START_1 = 0x42;
    constexpr uint8_t FRAME_START_2 = 0x4D;
    constexpr uint8_t PAYLOAD_SIZE = 28;  // 13 data words + checksum word

    const uint8_t COMMAND_SLEEP[] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
    const uint8_t COMMAND_WAKEUP[] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};

    enum class State : uint8_t {
      WarmingUp,  // fan running, readings not trustworthy yet
      Sampling,   // fan running, averaging frames
      Sleeping    // fan stopped
    };

    SoftwareSerial serial;

    State state = State::WarmingUp;
    uint32_t stateEnteredMs = 0;

    Data lastData;
    bool hasData = false;
    uint32_t lastDataMs = 0;

    // Frame parser state
    uint8_t payload[PAYLOAD_SIZE];
    uint8_t index = 0;
    uint16_t frameLength = 0;
    uint16_t checksum = 0;

    // Accumulator for the current sampling window
    uint32_t sampleSum[9] = {0};
    uint16_t sampleCount = 0;

    uint16_t word16(uint8_t high, uint8_t low) {
      return static_cast<uint16_t>((high << 8) | low);
    }

    void enterState(State next) {
      state = next;
      stateEnteredMs = millis();
    }

    void resetSamples() {
      memset(sampleSum, 0, sizeof(sampleSum));
      sampleCount = 0;
    }

    void accumulate(const uint16_t values[9]) {
      for (uint8_t i = 0; i < 9; ++i)
        sampleSum[i] += values[i];

      ++sampleCount;
    }

    void publishAverage() {
      if (sampleCount == 0)
        return;

      lastData.pm1 = sampleSum[0] / sampleCount;
      lastData.pm25 = sampleSum[1] / sampleCount;
      lastData.pm10 = sampleSum[2] / sampleCount;
      lastData.count03 = sampleSum[3] / sampleCount;
      lastData.count05 = sampleSum[4] / sampleCount;
      lastData.count10 = sampleSum[5] / sampleCount;
      lastData.count25 = sampleSum[6] / sampleCount;
      lastData.count50 = sampleSum[7] / sampleCount;
      lastData.count100 = sampleSum[8] / sampleCount;

      hasData = true;
      lastDataMs = millis();
    }

    // Feeds one byte to the frame parser, returns true on a complete valid frame.
    bool parseByte(uint8_t byteRead) {
      switch (index) {
        case 0:
          if (byteRead != FRAME_START_1)
            return false;

          checksum = byteRead;
          break;

        case 1:
          if (byteRead != FRAME_START_2) {
            index = 0;
            return false;
          }

          checksum += byteRead;
          break;

        case 2:
          checksum += byteRead;
          frameLength = static_cast<uint16_t>(byteRead << 8);
          break;

        case 3:
          frameLength |= byteRead;
          // PMS5003 announces 28 bytes; anything else is a transmission error
          if (frameLength != PAYLOAD_SIZE) {
            index = 0;
            return false;
          }

          checksum += byteRead;
          break;

        default: {
          const uint8_t payloadIndex = index - 4;
          payload[payloadIndex] = byteRead;

          // The last two payload bytes carry the checksum itself
          if (payloadIndex < PAYLOAD_SIZE - 2) {
            checksum += byteRead;
            break;
          }

          if (payloadIndex == PAYLOAD_SIZE - 1) {
            index = 0;
            return checksum == word16(payload[PAYLOAD_SIZE - 2], payload[PAYLOAD_SIZE - 1]);
          }

          break;
        }
      }

      ++index;
      return false;
    }

    void drainSerial() {
      while (serial.available()) {
        if (!parseByte(static_cast<uint8_t>(serial.read())))
          continue;

        // Bytes 0-5 are the CF=1 standard particle values, which we skip.
        const uint16_t values[9] = {
          word16(payload[6], payload[7]),    // PM1.0 atmospheric
          word16(payload[8], payload[9]),    // PM2.5 atmospheric
          word16(payload[10], payload[11]),  // PM10 atmospheric
          word16(payload[12], payload[13]),  // > 0.3 um count
          word16(payload[14], payload[15]),
          word16(payload[16], payload[17]),
          word16(payload[18], payload[19]),
          word16(payload[20], payload[21]),
          word16(payload[22], payload[23]),
        };

        if (state == State::Sampling)
          accumulate(values);
      }
    }

    void sendCommand(const uint8_t *command, size_t length) {
      serial.write(command, length);
      serial.flush();
    }
  }

  void begin() {
    serial.begin(9600, SWSERIAL_8N1, PIN_PMS_RX, PIN_PMS_TX, false, 256);
    sendCommand(COMMAND_WAKEUP, sizeof(COMMAND_WAKEUP));
    resetSamples();
    enterState(State::WarmingUp);
  }

  void loop() {
    drainSerial();

    const uint32_t elapsed = millis() - stateEnteredMs;

    switch (state) {
      case State::WarmingUp:
        if (elapsed >= PMS_WARMUP_MS) {
          resetSamples();
          enterState(State::Sampling);
        }
        break;

      case State::Sampling:
        if (elapsed >= PMS_SAMPLE_MS) {
          publishAverage();
          sendCommand(COMMAND_SLEEP, sizeof(COMMAND_SLEEP));
          enterState(State::Sleeping);
        }
        break;

      case State::Sleeping:
        if (elapsed >= PMS_SLEEP_MS) {
          sendCommand(COMMAND_WAKEUP, sizeof(COMMAND_WAKEUP));
          index = 0;
          enterState(State::WarmingUp);
        }
        break;
    }
  }

  bool hasReading() {
    return hasData && readingAgeMs() < PMS_READING_MAX_AGE_MS;
  }

  const Data &data() {
    return lastData;
  }

  uint32_t readingAgeMs() {
    return hasData ? millis() - lastDataMs : UINT32_MAX;
  }

  bool isFanRunning() {
    return state != State::Sleeping;
  }
}
