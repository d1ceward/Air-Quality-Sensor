#include <SoftwareSerial.h>

// Local includes
#include "config.h"
#include "sensors/pms5003.h"
#include "sensors/pms_frame.h"

namespace Pms5003 {
  namespace {
    const uint8_t COMMAND_SLEEP[] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
    const uint8_t COMMAND_WAKEUP[] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};

    enum class State : uint8_t {
      WarmingUp,  // fan running, readings not trustworthy yet
      Sampling,   // fan running, averaging frames
      Sleeping    // fan stopped
    };

    SoftwareSerial serial;
    PmsFrameParser parser;

    State state = State::WarmingUp;
    uint32_t stateEnteredMs = 0;

    Data lastData;
    bool hasData = false;
    uint32_t lastDataMs = 0;

    // Accumulator for the current sampling window
    uint32_t sampleSum[9] = {0};
    uint16_t sampleCount = 0;

    void enterState(State next) {
      state = next;
      stateEnteredMs = millis();
    }

    void resetSamples() {
      memset(sampleSum, 0, sizeof(sampleSum));
      sampleCount = 0;
    }

    void accumulate(const PmsFrameParser::Frame &frame) {
      const uint16_t values[9] = {
        frame.pm1, frame.pm25, frame.pm10,
        frame.count03, frame.count05, frame.count10,
        frame.count25, frame.count50, frame.count100,
      };

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

    void drainSerial() {
      while (serial.available()) {
        if (!parser.feed(static_cast<uint8_t>(serial.read())))
          continue;

        if (state == State::Sampling)
          accumulate(parser.frame());
      }
    }

    void sendCommand(const uint8_t *command, size_t length) {
      // No flush() here: on EspSoftwareSerial it discards the receive buffer instead of draining the transmit
      // one, and write() already blocks until the last stop bit.
      serial.write(command, length);
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
          parser.reset();
          enterState(State::WarmingUp);
        }
        break;
    }
  }

  bool isAvailable() {
    return hasData;
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
