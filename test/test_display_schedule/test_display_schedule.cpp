#include <unity.h>

#include "display_schedule.h"

void setUp() {}
void tearDown() {}

// The usual case: the window straddles midnight
void test_window_wrapping_past_midnight() {
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(22, 22, 7));
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(23, 22, 7));
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(0, 22, 7));
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(6, 22, 7));

  TEST_ASSERT_FALSE(DisplaySchedule::isNight(7, 22, 7));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(12, 22, 7));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(21, 22, 7));
}

// A window inside a single day, for an office that empties at lunchtime
void test_window_inside_one_day() {
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(13, 12, 14));
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(12, 12, 14));

  TEST_ASSERT_FALSE(DisplaySchedule::isNight(14, 12, 14));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(11, 12, 14));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(23, 12, 14));
}

// The start hour belongs to the window, the end hour does not
void test_bounds_are_half_open() {
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(1, 1, 2));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(2, 1, 2));
}

// Equal bounds mean the feature is off, not a full day of darkness
void test_equal_bounds_disable_the_window() {
  for (uint8_t hour = 0; hour < 24; ++hour)
    TEST_ASSERT_FALSE(DisplaySchedule::isNight(hour, 0, 0));

  for (uint8_t hour = 0; hour < 24; ++hour)
    TEST_ASSERT_FALSE(DisplaySchedule::isNight(hour, 22, 22));
}

// A corrupted configuration must not blank the display
void test_out_of_range_values_are_refused() {
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(25, 22, 7));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(3, 24, 7));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(3, 22, 99));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(255, 255, 255));
}

// Midnight to midday, a window that starts exactly at hour zero
void test_window_starting_at_midnight() {
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(0, 0, 12));
  TEST_ASSERT_TRUE(DisplaySchedule::isNight(11, 0, 12));
  TEST_ASSERT_FALSE(DisplaySchedule::isNight(12, 0, 12));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_window_wrapping_past_midnight);
  RUN_TEST(test_window_inside_one_day);
  RUN_TEST(test_bounds_are_half_open);
  RUN_TEST(test_equal_bounds_disable_the_window);
  RUN_TEST(test_out_of_range_values_are_refused);
  RUN_TEST(test_window_starting_at_midnight);
  return UNITY_END();
}
