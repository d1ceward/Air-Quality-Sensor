#include <SoftwareSerial.h>

// Local includes
#include "config.h"
#include "console.h"
#include "modbus_crc.h"
#include "sensors/senseair_s8.h"

namespace SenseAirS8 {
  namespace {
    constexpr uint8_t RESPONSE_SIZE = 7;
    constexpr uint32_t RESPONSE_TIMEOUT_MS = 500U;

    // Read one input register at address 3 (CO2 in ppm), device address 0xFE
    const uint8_t COMMAND_READ_CO2[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};
    // Clear the acknowledgement flags (holding register 0)
    const uint8_t COMMAND_CLEAR_ACK[] = {0xFE, 0x06, 0x00, 0x00, 0x00, 0x00, 0x9D, 0xC5};
    // Write 0x7C06 to holding register 1: start CO2 background calibration
    const uint8_t COMMAND_CALIBRATE[] = {0xFE, 0x06, 0x00, 0x01, 0x7C, 0x06, 0x6C, 0xC7};

    SoftwareSerial serial;

    uint16_t lastCo2 = 0;
    bool hasCo2 = false;
    uint32_t lastCo2Ms = 0;

    bool awaitingResponse = false;
    uint32_t requestSentMs = 0;
    uint32_t lastRequestMs = 0;

    uint8_t response[RESPONSE_SIZE];
    uint8_t responseLength = 0;

    void flushInput() {
      while (serial.available())
        serial.read();
    }

    // Without the bytes themselves, a failure cannot be told apart from silence
    void reportFailure(const __FlashStringHelper *reason) {
      // print() knows how to walk a flash string; printf("%s") would read it with unaligned accesses and
      // fault.
      Console.print(F("[co2] "));
      Console.print(reason);
      Console.printf(" (%u/%u bytes:", responseLength, RESPONSE_SIZE);

      for (uint8_t i = 0; i < responseLength; ++i)
        Console.printf(" %02X", response[i]);

      Console.println(")");
    }

    void sendRequest() {
      flushInput();
      responseLength = 0;
      // write() blocks until the last stop bit, and flush() must not be called here: on EspSoftwareSerial it
      // discards the receive buffer rather than draining the transmit one, which can eat the start of the
      // answer.
      serial.write(COMMAND_READ_CO2, sizeof(COMMAND_READ_CO2));
      requestSentMs = millis();
      lastRequestMs = requestSentMs;
      awaitingResponse = true;
    }

    void handleResponse() {
      while (serial.available() && responseLength < RESPONSE_SIZE)
        response[responseLength++] = static_cast<uint8_t>(serial.read());

      if (responseLength < RESPONSE_SIZE) {
        if (millis() - requestSentMs >= RESPONSE_TIMEOUT_MS) {
          reportFailure(F("response timeout"));
          awaitingResponse = false;
        }

        return;
      }

      awaitingResponse = false;

      const uint16_t expectedCrc = modbusCrc16(response, RESPONSE_SIZE - 2);
      const uint16_t receivedCrc = static_cast<uint16_t>(response[6] << 8 | response[5]);

      if (response[0] != 0xFE || response[1] != 0x04 || response[2] != 0x02) {
        reportFailure(F("unexpected response header"));
        return;
      }

      if (expectedCrc != receivedCrc) {
        reportFailure(F("CRC mismatch"));
        return;
      }

      lastCo2 = static_cast<uint16_t>(response[3] << 8 | response[4]);

      // The first valid answer confirms the wiring, the rest stays quiet
      if (!hasCo2)
        Console.printf("[co2] sensor answering, %u ppm\n", lastCo2);

      hasCo2 = true;
      lastCo2Ms = millis();
    }
  }

  void begin() {
    serial.begin(9600, SWSERIAL_8N1, PIN_CO2_RX, PIN_CO2_TX, false, 64);
    // First request happens on the next loop() call
    lastRequestMs = millis() - CO2_INTERVAL_MS;
  }

  void loop() {
    if (awaitingResponse) {
      handleResponse();
      return;
    }

    if (millis() - lastRequestMs >= CO2_INTERVAL_MS)
      sendRequest();
  }

  bool isAvailable() {
    return hasCo2;
  }

  bool hasReading() {
    return hasCo2 && readingAgeMs() < READING_MAX_AGE_MS;
  }

  uint16_t co2Ppm() {
    return lastCo2;
  }

  uint32_t readingAgeMs() {
    return hasCo2 ? millis() - lastCo2Ms : UINT32_MAX;
  }

  bool calibrate() {
    flushInput();
    awaitingResponse = false;
    responseLength = 0;

    // The sensor expects the acknowledgement flags to be cleared first
    serial.write(COMMAND_CLEAR_ACK, sizeof(COMMAND_CLEAR_ACK));
    delay(50);
    flushInput();

    const size_t written = serial.write(COMMAND_CALIBRATE, sizeof(COMMAND_CALIBRATE));
    Console.println(F("[co2] background calibration requested"));

    return written == sizeof(COMMAND_CALIBRATE);
  }
}
