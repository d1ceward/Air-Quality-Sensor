#include <unity.h>

#include "aqi.h"

void setUp() {}
void tearDown() {}

// Values checked against the US EPA PM2.5 breakpoint table revised in February 2024
void test_breakpoint_boundaries() {
  TEST_ASSERT_EQUAL_UINT16(0, Aqi::fromPm25Us(0));
  TEST_ASSERT_EQUAL_UINT16(50, Aqi::fromPm25Us(9));
  TEST_ASSERT_EQUAL_UINT16(99, Aqi::fromPm25Us(35));
  TEST_ASSERT_EQUAL_UINT16(149, Aqi::fromPm25Us(55));
  TEST_ASSERT_EQUAL_UINT16(200, Aqi::fromPm25Us(125));
  TEST_ASSERT_EQUAL_UINT16(300, Aqi::fromPm25Us(225));
  TEST_ASSERT_EQUAL_UINT16(499, Aqi::fromPm25Us(325));
  TEST_ASSERT_EQUAL_UINT16(500, Aqi::fromPm25Us(326));
}

void test_interpolates_inside_a_band() {
  TEST_ASSERT_EQUAL_UINT16(28, Aqi::fromPm25Us(5));
  TEST_ASSERT_EQUAL_UINT16(53, Aqi::fromPm25Us(10));
  TEST_ASSERT_EQUAL_UINT16(102, Aqi::fromPm25Us(36));
  TEST_ASSERT_EQUAL_UINT16(151, Aqi::fromPm25Us(56));
  TEST_ASSERT_EQUAL_UINT16(302, Aqi::fromPm25Us(226));
}

// The 2024 revision moved the first two bands: 12 ug/m3 used to sit exactly on 50
void test_uses_the_2024_breakpoints() {
  TEST_ASSERT_EQUAL_UINT16(56, Aqi::fromPm25Us(12));
  // Unhealthy now starts at 55.5 instead of running to 150.4
  TEST_ASSERT_EQUAL_UINT16(201, Aqi::fromPm25Us(126));
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
  RUN_TEST(test_uses_the_2024_breakpoints);
  RUN_TEST(test_clamps_above_the_last_breakpoint);
  RUN_TEST(test_is_monotonic);
  return UNITY_END();
}
