#include <unity.h>

#include "battery_curve.h"

void setUp() {}
void tearDown() {}

void test_reports_the_ends_of_the_curve() {
  TEST_ASSERT_EQUAL_UINT8(100, BatteryCurve::percentFromVolts(4.20f));
  TEST_ASSERT_EQUAL_UINT8(0, BatteryCurve::percentFromVolts(3.00f));
}

// A cell out of range must not wrap around on an unsigned type
void test_clamps_outside_the_curve() {
  TEST_ASSERT_EQUAL_UINT8(100, BatteryCurve::percentFromVolts(4.35f));
  TEST_ASSERT_EQUAL_UINT8(100, BatteryCurve::percentFromVolts(12.0f));
  TEST_ASSERT_EQUAL_UINT8(0, BatteryCurve::percentFromVolts(2.5f));
  TEST_ASSERT_EQUAL_UINT8(0, BatteryCurve::percentFromVolts(0.0f));
  TEST_ASSERT_EQUAL_UINT8(0, BatteryCurve::percentFromVolts(-1.0f));
}

void test_returns_the_tabulated_points() {
  TEST_ASSERT_EQUAL_UINT8(90, BatteryCurve::percentFromVolts(4.10f));
  TEST_ASSERT_EQUAL_UINT8(50, BatteryCurve::percentFromVolts(3.82f));
  TEST_ASSERT_EQUAL_UINT8(10, BatteryCurve::percentFromVolts(3.68f));
}

void test_interpolates_between_two_points() {
  // Halfway between 4.00 V at 80 % and 4.10 V at 90 %
  TEST_ASSERT_EQUAL_UINT8(85, BatteryCurve::percentFromVolts(4.05f));
  // Halfway between 3.79 V at 40 % and 3.82 V at 50 %
  TEST_ASSERT_EQUAL_UINT8(45, BatteryCurve::percentFromVolts(3.805f));
}

// The curve is what makes the reading useful: a linear reading would still be
// above 60 % at 3.75 V, where the cell is nearly empty
void test_is_not_linear() {
  const uint8_t atMidVoltage = BatteryCurve::percentFromVolts(3.60f);
  TEST_ASSERT_LESS_THAN_UINT8(10, atMidVoltage);
}

// The charge never rises as the voltage falls
void test_is_monotonic() {
  uint8_t previous = 100;

  for (int millivolts = 4300; millivolts >= 2500; millivolts -= 5) {
    const uint8_t percent = BatteryCurve::percentFromVolts(millivolts / 1000.0f);
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(previous, percent);
    previous = percent;
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reports_the_ends_of_the_curve);
  RUN_TEST(test_clamps_outside_the_curve);
  RUN_TEST(test_returns_the_tabulated_points);
  RUN_TEST(test_interpolates_between_two_points);
  RUN_TEST(test_is_not_linear);
  RUN_TEST(test_is_monotonic);
  return UNITY_END();
}
