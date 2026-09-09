#include <unity.h>

#include "uptime.h"

void setUp() {
  Uptime::reset();
}

void tearDown() {}

void test_follows_the_raw_counter_before_any_wrap() {
  TEST_ASSERT_EQUAL_UINT64(0, Uptime::tick(0));
  TEST_ASSERT_EQUAL_UINT64(1500, Uptime::tick(1500));
  TEST_ASSERT_EQUAL_UINT32(1, Uptime::seconds());
}

// 49.71 days of uptime used to report as zero
void test_keeps_counting_across_a_wrap() {
  Uptime::tick(UINT32_MAX - 1000);
  const uint64_t afterWrap = Uptime::tick(2000);

  TEST_ASSERT_EQUAL_UINT64(0x100000000ULL + 2000ULL, afterWrap);
  TEST_ASSERT_EQUAL_UINT32(4294969, Uptime::seconds());
}

void test_counts_several_wraps() {
  for (uint8_t wrap = 0; wrap < 3; ++wrap) {
    Uptime::tick(UINT32_MAX - 10);
    Uptime::tick(10);
  }

  TEST_ASSERT_EQUAL_UINT64((3ULL << 32) + 10ULL, Uptime::millis64());
}

// A first call with an already large counter must not be read as a wrap
void test_does_not_wrap_on_the_first_call() {
  TEST_ASSERT_EQUAL_UINT64(4000000000ULL, Uptime::tick(4000000000UL));
}

void test_never_goes_backwards() {
  uint64_t previous = 0;
  uint32_t ticks = 0;

  // Steps of 40 days, so the counter wraps more than once over the run
  for (uint16_t step = 0; step < 500; ++step) {
    ticks += 3456000000UL;
    const uint64_t extended = Uptime::tick(ticks);
    TEST_ASSERT_TRUE(extended >= previous);
    previous = extended;
  }
}

void test_reset_drops_the_accumulated_wraps() {
  Uptime::tick(UINT32_MAX - 5);
  Uptime::tick(5);
  Uptime::reset();

  TEST_ASSERT_EQUAL_UINT64(0, Uptime::millis64());
  TEST_ASSERT_EQUAL_UINT64(700, Uptime::tick(700));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_follows_the_raw_counter_before_any_wrap);
  RUN_TEST(test_keeps_counting_across_a_wrap);
  RUN_TEST(test_counts_several_wraps);
  RUN_TEST(test_does_not_wrap_on_the_first_call);
  RUN_TEST(test_never_goes_backwards);
  RUN_TEST(test_reset_drops_the_accumulated_wraps);
  return UNITY_END();
}
