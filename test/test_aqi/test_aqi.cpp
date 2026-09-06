#include <unity.h>

#include "aqi.h"

void setUp() {}
void tearDown() {}

// Values checked against the US EPA PM2.5 breakpoint table
void test_breakpoint_boundaries() {
  TEST_ASSERT_EQUAL_UINT16(0, Aqi::fromPm25Us(0));
  TEST_ASSERT_EQUAL_UINT16(50, Aqi::fromPm25Us(12));
  TEST_ASSERT_EQUAL_UINT16(99, Aqi::fromPm25Us(35));
  TEST_ASSERT_EQUAL_UINT16(149, Aqi::fromPm25Us(55));
  TEST_ASSERT_EQUAL_UINT16(200, Aqi::fromPm25Us(150));
  TEST_ASSERT_EQUAL_UINT16(300, Aqi::fromPm25Us(250));
  TEST_ASSERT_EQUAL_UINT16(400, Aqi::fromPm25Us(350));
  TEST_ASSERT_EQUAL_UINT16(500, Aqi::fromPm25Us(500));
}

void test_interpolates_inside_a_band() {
  TEST_ASSERT_EQUAL_UINT16(25, Aqi::fromPm25Us(6));
  TEST_ASSERT_EQUAL_UINT16(53, Aqi::fromPm25Us(13));
  TEST_ASSERT_EQUAL_UINT16(102, Aqi::fromPm25Us(36));
}

// The scale stops at 500, a sensor spike must not wrap around
void test_clamps_above_the_last_breakpoint() {
  TEST_ASSERT_EQUAL_UINT16(500, Aqi::fromPm25Us(501));
  TEST_ASSERT_EQUAL_UINT16(500, Aqi::fromPm25Us(999));
  TEST_ASSERT_EQUAL_UINT16(500, Aqi::fromPm25Us(65535));
}

// The index never decreases as the concentration rises
void test_is_monotonic() {
  uint16_t previous = 0;

  for (uint16_t pm25 = 0; pm25 <= 600; ++pm25) {
    const uint16_t index = Aqi::fromPm25Us(pm25);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(previous, index);
    previous = index;
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_breakpoint_boundaries);
  RUN_TEST(test_interpolates_inside_a_band);
  RUN_TEST(test_clamps_above_the_last_breakpoint);
  RUN_TEST(test_is_monotonic);
  return UNITY_END();
}
