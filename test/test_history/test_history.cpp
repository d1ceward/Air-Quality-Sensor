#include <math.h>
#include <unity.h>

#include "history.h"

namespace {
  History::Ring ring;

  // Pushes one sample where every series carries the same value, so a slot is easy to identify
  void pushUniform(int16_t value) {
    int16_t values[History::SERIES_COUNT];
    for (uint8_t series = 0; series < History::SERIES_COUNT; ++series)
      values[series] = value;

    ring.push(values);
  }
}

void setUp() {
  ring.clear();
}

void tearDown() {}

void test_starts_empty() {
  TEST_ASSERT_EQUAL_UINT16(0, ring.size());
  TEST_ASSERT_FALSE(ring.isFull());
  TEST_ASSERT_EQUAL_INT16(History::NO_SAMPLE, ring.at(History::SERIES_CO2, 0));
}

void test_reads_back_in_push_order() {
  pushUniform(10);
  pushUniform(20);
  pushUniform(30);

  TEST_ASSERT_EQUAL_UINT16(3, ring.size());
  TEST_ASSERT_EQUAL_INT16(10, ring.at(History::SERIES_CO2, 0));
  TEST_ASSERT_EQUAL_INT16(20, ring.at(History::SERIES_CO2, 1));
  TEST_ASSERT_EQUAL_INT16(30, ring.at(History::SERIES_CO2, 2));
}

void test_keeps_series_apart() {
  int16_t values[History::SERIES_COUNT];
  for (uint8_t series = 0; series < History::SERIES_COUNT; ++series)
    values[series] = static_cast<int16_t>(100 + series);

  ring.push(values);

  TEST_ASSERT_EQUAL_INT16(100, ring.at(History::SERIES_PM25, 0));
  TEST_ASSERT_EQUAL_INT16(103, ring.at(History::SERIES_TEMPERATURE, 0));
  TEST_ASSERT_EQUAL_INT16(106, ring.at(History::SERIES_BATTERY, 0));
}

// The window slides rather than growing: the oldest sample goes when a new one arrives
void test_drops_the_oldest_once_full() {
  for (uint16_t index = 0; index < History::CAPACITY; ++index)
    pushUniform(static_cast<int16_t>(index));

  TEST_ASSERT_TRUE(ring.isFull());
  TEST_ASSERT_EQUAL_INT16(0, ring.at(History::SERIES_PM25, 0));

  pushUniform(9999);

  TEST_ASSERT_EQUAL_UINT16(History::CAPACITY, ring.size());
  TEST_ASSERT_EQUAL_INT16(1, ring.at(History::SERIES_PM25, 0));
  TEST_ASSERT_EQUAL_INT16(9999, ring.at(History::SERIES_PM25, History::CAPACITY - 1));
}

void test_stays_ordered_after_many_wraps() {
  for (uint16_t index = 0; index < History::CAPACITY * 3 + 7; ++index)
    pushUniform(static_cast<int16_t>(index % 1000));

  int16_t previous = ring.at(History::SERIES_TVOC, 0);
  for (uint16_t index = 1; index < ring.size(); ++index) {
    const int16_t current = ring.at(History::SERIES_TVOC, index);
    TEST_ASSERT_EQUAL_INT16((previous + 1) % 1000, current);
    previous = current;
  }
}

void test_read_copies_a_series() {
  pushUniform(1);
  pushUniform(2);

  int16_t out[4] = {-1, -1, -1, -1};
  TEST_ASSERT_EQUAL_UINT16(2, ring.read(History::SERIES_HUMIDITY, out, 4));
  TEST_ASSERT_EQUAL_INT16(1, out[0]);
  TEST_ASSERT_EQUAL_INT16(2, out[1]);
}

void test_read_respects_the_output_size() {
  for (uint16_t index = 0; index < 10; ++index)
    pushUniform(static_cast<int16_t>(index));

  int16_t out[3];
  TEST_ASSERT_EQUAL_UINT16(3, ring.read(History::SERIES_PM25, out, 3));
  TEST_ASSERT_EQUAL_INT16(0, out[0]);
}

void test_rejects_an_unknown_series() {
  pushUniform(5);

  int16_t out[2];
  TEST_ASSERT_EQUAL_UINT16(0, ring.read(History::SERIES_COUNT, out, 2));
  TEST_ASSERT_EQUAL_INT16(History::NO_SAMPLE, ring.at(History::SERIES_COUNT, 0));
  TEST_ASSERT_EQUAL_UINT16(0, ring.read(History::SERIES_PM25, nullptr, 2));
}

// A gap has to stay a gap, otherwise the sparkline draws a line down to zero through it
void test_scaled_marks_an_invalid_reading() {
  TEST_ASSERT_EQUAL_INT16(History::NO_SAMPLE, History::scaled(false, 21.5f, 10.0f));
  TEST_ASSERT_EQUAL_INT16(History::NO_SAMPLE, History::scaled(true, NAN, 10.0f));
  TEST_ASSERT_EQUAL_INT16(History::NO_SAMPLE, History::clamped(false, 400));
}

void test_scaled_rounds_to_the_nearest() {
  TEST_ASSERT_EQUAL_INT16(215, History::scaled(true, 21.54f, 10.0f));
  TEST_ASSERT_EQUAL_INT16(216, History::scaled(true, 21.56f, 10.0f));
  TEST_ASSERT_EQUAL_INT16(-45, History::scaled(true, -4.5f, 10.0f));
  // Exactly on a half, rounded away from zero
  TEST_ASSERT_EQUAL_INT16(10133, History::scaled(true, 1013.25f, 10.0f));
}

// The SGP30 reports TVOC up to 60000 ppb, which would wrap a slot
void test_clamps_instead_of_wrapping() {
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, History::clamped(true, 60000));
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, History::scaled(true, 99999.0f, 10.0f));
  TEST_ASSERT_EQUAL_INT16(INT16_MIN + 1, History::scaled(true, -99999.0f, 10.0f));
  TEST_ASSERT_NOT_EQUAL(History::NO_SAMPLE, History::scaled(true, -99999.0f, 10.0f));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_empty);
  RUN_TEST(test_reads_back_in_push_order);
  RUN_TEST(test_keeps_series_apart);
  RUN_TEST(test_drops_the_oldest_once_full);
  RUN_TEST(test_stays_ordered_after_many_wraps);
  RUN_TEST(test_read_copies_a_series);
  RUN_TEST(test_read_respects_the_output_size);
  RUN_TEST(test_rejects_an_unknown_series);
  RUN_TEST(test_scaled_marks_an_invalid_reading);
  RUN_TEST(test_scaled_rounds_to_the_nearest);
  RUN_TEST(test_clamps_instead_of_wrapping);
  return UNITY_END();
}
