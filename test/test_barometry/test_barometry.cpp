#include <unity.h>

#include "barometry.h"

void setUp() {}
void tearDown() {}

// Roughly 12 hPa per 100 m near sea level, less linear further up
void test_reduces_pressure_to_sea_level() {
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 1011.938f, Barometry::toSeaLevelHpa(1000.0f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 1061.421f, Barometry::toSeaLevelHpa(1000.0f, 500.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 1127.384f, Barometry::toSeaLevelHpa(1000.0f, 1000.0f));
}

// A station at sea level reports its reading unchanged
void test_leaves_a_sea_level_station_alone() {
  TEST_ASSERT_EQUAL_FLOAT(1000.0f, Barometry::toSeaLevelHpa(1000.0f, 0.0f));
}

void test_derives_altitude_from_the_standard_atmosphere() {
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, Barometry::toAltitudeMeters(Barometry::SEA_LEVEL_PRESSURE_HPA));
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 110.9f, Barometry::toAltitudeMeters(1000.0f));
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 1000.6f, Barometry::toAltitudeMeters(898.7f));
}

// A failed conversion reads zero, which must not become a negative altitude
void test_handles_an_invalid_pressure() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, Barometry::toAltitudeMeters(0.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, Barometry::toAltitudeMeters(-5.0f));
}

// The two conversions are inverses of each other
void test_round_trips_through_sea_level() {
  const float measured = 950.0f;
  const float altitude = Barometry::toAltitudeMeters(measured);
  const float reduced = Barometry::toSeaLevelHpa(measured, altitude);

  TEST_ASSERT_FLOAT_WITHIN(0.5f, Barometry::SEA_LEVEL_PRESSURE_HPA, reduced);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reduces_pressure_to_sea_level);
  RUN_TEST(test_leaves_a_sea_level_station_alone);
  RUN_TEST(test_derives_altitude_from_the_standard_atmosphere);
  RUN_TEST(test_handles_an_invalid_pressure);
  RUN_TEST(test_round_trips_through_sea_level);
  return UNITY_END();
}
