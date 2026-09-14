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

// The curve is what makes the reading useful: a linear reading would still be above 60 % at 3.75 V, where the
// cell is nearly empty
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

// A charger cuts out at 4.2 V but the cell relaxes below it, so without a lower anchor a full battery reads
// 97 % and never gets to 100 %
void test_anchors_the_full_end_on_the_configured_voltage() {
  TEST_ASSERT_EQUAL_UINT8(97, BatteryCurve::percentFromVolts(4.17f));
  TEST_ASSERT_EQUAL_UINT8(100, BatteryCurve::percentFromVolts(4.17f, 4.15f));
  TEST_ASSERT_EQUAL_UINT8(100, BatteryCurve::percentFromVolts(4.15f, 4.15f));
}

// Halfway between 4.10 V at 90 % and the 4.15 V anchor at 100 %
void test_interpolates_against_the_anchor() {
  TEST_ASSERT_EQUAL_UINT8(95, BatteryCurve::percentFromVolts(4.125f, 4.15f));
}

// The chemistry below 90 % does not move with the anchor
void test_leaves_the_rest_of_the_curve_alone() {
  TEST_ASSERT_EQUAL_UINT8(90, BatteryCurve::percentFromVolts(4.10f, 4.15f));
  TEST_ASSERT_EQUAL_UINT8(85, BatteryCurve::percentFromVolts(4.05f, 4.15f));
  TEST_ASSERT_EQUAL_UINT8(50, BatteryCurve::percentFromVolts(3.82f, 4.15f));
  TEST_ASSERT_EQUAL_UINT8(0, BatteryCurve::percentFromVolts(3.00f, 4.15f));
}

// An anchor at or below the 90 % point would divide by a zero or negative span
void test_clamps_an_anchor_under_the_top_point() {
  TEST_ASSERT_EQUAL_UINT8(100, BatteryCurve::percentFromVolts(4.12f, 3.00f));
  TEST_ASSERT_EQUAL_UINT8(80, BatteryCurve::percentFromVolts(4.00f, 4.00f));
  TEST_ASSERT_EQUAL_UINT8(90, BatteryCurve::percentFromVolts(4.10f, 0.0f));
}

void test_is_monotonic_against_an_anchor() {
  uint8_t previous = 100;

  for (int millivolts = 4300; millivolts >= 2500; millivolts -= 5) {
    const uint8_t percent = BatteryCurve::percentFromVolts(millivolts / 1000.0f, 4.15f);
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(previous, percent);
    previous = percent;
  }
}

// A converter step is about 4.4 mV, which the curve turns into a couple of points around 3.8 V: the reading
// has to stay put through that much drift
void test_holds_the_reading_inside_the_deadband() {
  TEST_ASSERT_EQUAL_UINT8(45, BatteryCurve::stablePercent(45, 46, 3));
  TEST_ASSERT_EQUAL_UINT8(45, BatteryCurve::stablePercent(45, 43, 3));
  TEST_ASSERT_EQUAL_UINT8(45, BatteryCurve::stablePercent(45, 45, 3));
}

void test_follows_a_move_past_the_deadband() {
  TEST_ASSERT_EQUAL_UINT8(48, BatteryCurve::stablePercent(45, 48, 3));
  TEST_ASSERT_EQUAL_UINT8(42, BatteryCurve::stablePercent(45, 42, 3));
  TEST_ASSERT_EQUAL_UINT8(90, BatteryCurve::stablePercent(45, 90, 3));
}

// The gauge would otherwise stop a few points short of either end
void test_always_reports_the_ends() {
  TEST_ASSERT_EQUAL_UINT8(100, BatteryCurve::stablePercent(99, 100, 3));
  TEST_ASSERT_EQUAL_UINT8(0, BatteryCurve::stablePercent(2, 0, 3));
}

// A deadband of 0 must not pin the reading to its first value
void test_passes_everything_through_without_a_deadband() {
  TEST_ASSERT_EQUAL_UINT8(46, BatteryCurve::stablePercent(45, 46, 0));
  TEST_ASSERT_EQUAL_UINT8(45, BatteryCurve::stablePercent(45, 45, 0));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reports_the_ends_of_the_curve);
  RUN_TEST(test_clamps_outside_the_curve);
  RUN_TEST(test_returns_the_tabulated_points);
  RUN_TEST(test_interpolates_between_two_points);
  RUN_TEST(test_is_not_linear);
  RUN_TEST(test_is_monotonic);
  RUN_TEST(test_anchors_the_full_end_on_the_configured_voltage);
  RUN_TEST(test_interpolates_against_the_anchor);
  RUN_TEST(test_leaves_the_rest_of_the_curve_alone);
  RUN_TEST(test_clamps_an_anchor_under_the_top_point);
  RUN_TEST(test_is_monotonic_against_an_anchor);
  RUN_TEST(test_holds_the_reading_inside_the_deadband);
  RUN_TEST(test_follows_a_move_past_the_deadband);
  RUN_TEST(test_always_reports_the_ends);
  RUN_TEST(test_passes_everything_through_without_a_deadband);
  return UNITY_END();
}
