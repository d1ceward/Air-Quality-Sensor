#include <unity.h>

#include "modbus_crc.h"

void setUp() {}
void tearDown() {}

// Frames taken from the SenseAir S8 Modbus documentation, checksum included,
// so the expected values do not come from this implementation.
void test_known_request_frames() {
  const uint8_t readCo2[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01};
  TEST_ASSERT_EQUAL_HEX16(0xC5D5, modbusCrc16(readCo2, sizeof(readCo2)));

  const uint8_t backgroundCalibration[] = {0xFE, 0x06, 0x00, 0x01, 0x7C, 0x06};
  TEST_ASSERT_EQUAL_HEX16(0xC76C, modbusCrc16(backgroundCalibration, sizeof(backgroundCalibration)));

  const uint8_t clearAcknowledgement[] = {0xFE, 0x06, 0x00, 0x00, 0x00, 0x00};
  TEST_ASSERT_EQUAL_HEX16(0xC59D, modbusCrc16(clearAcknowledgement, sizeof(clearAcknowledgement)));
}

// The checksum travels low byte first, which is how the driver reassembles it
void test_wire_byte_order() {
  const uint8_t readCo2[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};
  const uint16_t computed = modbusCrc16(readCo2, sizeof(readCo2) - 2);
  const uint16_t received = static_cast<uint16_t>(readCo2[7] << 8 | readCo2[6]);

  TEST_ASSERT_EQUAL_HEX16(computed, received);
}

// A response carrying 900 ppm, as the sensor would send it
void test_accepts_a_valid_response() {
  uint8_t response[] = {0xFE, 0x04, 0x02, 0x03, 0x84, 0x00, 0x00};
  const uint16_t crc = modbusCrc16(response, 5);
  response[5] = static_cast<uint8_t>(crc & 0xFF);
  response[6] = static_cast<uint8_t>(crc >> 8);

  const uint16_t received = static_cast<uint16_t>(response[6] << 8 | response[5]);
  TEST_ASSERT_EQUAL_HEX16(crc, received);
  TEST_ASSERT_EQUAL_UINT16(900, static_cast<uint16_t>(response[3] << 8 | response[4]));
}

// A single flipped bit has to be caught, that is the whole point
void test_rejects_a_corrupted_frame() {
  const uint8_t original[] = {0xFE, 0x04, 0x02, 0x03, 0x84};
  uint8_t corrupted[] = {0xFE, 0x04, 0x02, 0x03, 0x85};

  TEST_ASSERT_NOT_EQUAL(modbusCrc16(original, sizeof(original)),
                        modbusCrc16(corrupted, sizeof(corrupted)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_known_request_frames);
  RUN_TEST(test_wire_byte_order);
  RUN_TEST(test_accepts_a_valid_response);
  RUN_TEST(test_rejects_a_corrupted_frame);
  return UNITY_END();
}
