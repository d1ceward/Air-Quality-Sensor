#include <string.h>
#include <unity.h>

#include "sensors/pms_frame.h"

namespace {
  // A complete PMS5003 frame: header, length 0x001C, thirteen data words then
  // the checksum. Atmospheric values are 6, 14 and 20 ug/m3.
  const uint8_t VALID_FRAME[] = {
    0x42, 0x4D, 0x00, 0x1C,
    0x00, 0x05, 0x00, 0x0C, 0x00, 0x12,  // CF=1 standard particles
    0x00, 0x06, 0x00, 0x0E, 0x00, 0x14,  // atmospheric environment
    0x05, 0xDC, 0x01, 0xA4, 0x00, 0x78,  // counts above 0.3, 0.5, 1.0 um
    0x00, 0x1E, 0x00, 0x08, 0x00, 0x02,  // counts above 2.5, 5.0, 10 um
    0x97, 0x00,                          // reserved
    0x03, 0xB3,                          // checksum
  };

  // Feeds a buffer and returns how many complete frames came out
  uint8_t feedAll(PmsFrameParser &parser, const uint8_t *data, size_t length) {
    uint8_t frames = 0;

    for (size_t i = 0; i < length; ++i)
      if (parser.feed(data[i]))
        ++frames;

    return frames;
  }
}

void setUp() {}
void tearDown() {}

void test_decodes_a_valid_frame() {
  PmsFrameParser parser;

  TEST_ASSERT_EQUAL_UINT8(1, feedAll(parser, VALID_FRAME, sizeof(VALID_FRAME)));

  // The CF=1 values must not be mistaken for the atmospheric ones
  TEST_ASSERT_EQUAL_UINT16(6, parser.frame().pm1);
  TEST_ASSERT_EQUAL_UINT16(14, parser.frame().pm25);
  TEST_ASSERT_EQUAL_UINT16(20, parser.frame().pm10);
  TEST_ASSERT_EQUAL_UINT16(1500, parser.frame().count03);
  TEST_ASSERT_EQUAL_UINT16(420, parser.frame().count05);
  TEST_ASSERT_EQUAL_UINT16(120, parser.frame().count10);
  TEST_ASSERT_EQUAL_UINT16(30, parser.frame().count25);
  TEST_ASSERT_EQUAL_UINT16(8, parser.frame().count50);
  TEST_ASSERT_EQUAL_UINT16(2, parser.frame().count100);
}

// A corrupted byte must not reach the metrics
void test_rejects_a_bad_checksum() {
  PmsFrameParser parser;
  uint8_t corrupted[sizeof(VALID_FRAME)];
  memcpy(corrupted, VALID_FRAME, sizeof(VALID_FRAME));
  corrupted[9] = 0x0F;  // PM2.5 low byte, checksum left untouched

  TEST_ASSERT_EQUAL_UINT8(0, feedAll(parser, corrupted, sizeof(corrupted)));
}

// Waking the sensor mid-frame leaves partial data in the buffer
void test_resynchronises_after_garbage() {
  PmsFrameParser parser;
  const uint8_t garbage[] = {0x00, 0xFF, 0x4D, 0x42, 0x13, 0x42};

  feedAll(parser, garbage, sizeof(garbage));
  TEST_ASSERT_EQUAL_UINT8(1, feedAll(parser, VALID_FRAME, sizeof(VALID_FRAME)));
  TEST_ASSERT_EQUAL_UINT16(14, parser.frame().pm25);
}

// 0x42 right before the real header must not swallow it
void test_recovers_when_a_false_header_precedes_the_frame() {
  PmsFrameParser parser;
  const uint8_t falseStart[] = {0x42, 0x42};

  feedAll(parser, falseStart, sizeof(falseStart));
  TEST_ASSERT_EQUAL_UINT8(1, feedAll(parser, VALID_FRAME + 1, sizeof(VALID_FRAME) - 1));
  TEST_ASSERT_EQUAL_UINT16(14, parser.frame().pm25);
}

// Another Plantower model announcing a different payload size is not decoded
void test_rejects_an_unexpected_frame_length() {
  PmsFrameParser parser;
  uint8_t wrongLength[sizeof(VALID_FRAME)];
  memcpy(wrongLength, VALID_FRAME, sizeof(VALID_FRAME));
  wrongLength[3] = 0x14;

  TEST_ASSERT_EQUAL_UINT8(0, feedAll(parser, wrongLength, sizeof(wrongLength)));
}

void test_decodes_frames_back_to_back() {
  PmsFrameParser parser;

  TEST_ASSERT_EQUAL_UINT8(1, feedAll(parser, VALID_FRAME, sizeof(VALID_FRAME)));
  TEST_ASSERT_EQUAL_UINT8(1, feedAll(parser, VALID_FRAME, sizeof(VALID_FRAME)));
  TEST_ASSERT_EQUAL_UINT16(14, parser.frame().pm25);
}

// reset() is called when the fan wakes up, it must not leave a half frame
void test_reset_drops_a_partial_frame() {
  PmsFrameParser parser;

  feedAll(parser, VALID_FRAME, 10);
  parser.reset();
  TEST_ASSERT_EQUAL_UINT8(0, feedAll(parser, VALID_FRAME + 10, sizeof(VALID_FRAME) - 10));
  TEST_ASSERT_EQUAL_UINT8(1, feedAll(parser, VALID_FRAME, sizeof(VALID_FRAME)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_decodes_a_valid_frame);
  RUN_TEST(test_rejects_a_bad_checksum);
  RUN_TEST(test_resynchronises_after_garbage);
  RUN_TEST(test_recovers_when_a_false_header_precedes_the_frame);
  RUN_TEST(test_rejects_an_unexpected_frame_length);
  RUN_TEST(test_decodes_frames_back_to_back);
  RUN_TEST(test_reset_drops_a_partial_frame);
  return UNITY_END();
}
