#include <string.h>
#include <unity.h>

#include "hostname.h"

namespace {
  char buffer[80];

  size_t sanitize(const char *value) {
    return Hostname::sanitize(value, buffer, sizeof(buffer));
  }
}

void setUp() {
  memset(buffer, 0, sizeof(buffer));
}

void tearDown() {}

void test_keeps_a_valid_label_as_is() {
  TEST_ASSERT_EQUAL_size_t(11, sanitize("air-quality"));
  TEST_ASSERT_EQUAL_STRING("air-quality", buffer);
}

void test_folds_case_down() {
  sanitize("Air-Quality-01");
  TEST_ASSERT_EQUAL_STRING("air-quality-01", buffer);
}

void test_turns_separators_into_hyphens() {
  sanitize("living room_sensor.two");
  TEST_ASSERT_EQUAL_STRING("living-room-sensor-two", buffer);
}

void test_collapses_repeated_separators() {
  sanitize("living   room");
  TEST_ASSERT_EQUAL_STRING("living-room", buffer);
}

void test_drops_unsupported_characters() {
  sanitize("salon/chambre:2 (nord)");
  TEST_ASSERT_EQUAL_STRING("salonchambre2-nord", buffer);
}

// RFC 1123 forbids a label starting or ending with a hyphen, and some resolvers reject the record outright
void test_strips_leading_and_trailing_hyphens() {
  sanitize("-sensor-");
  TEST_ASSERT_EQUAL_STRING("sensor", buffer);

  sanitize("   sensor   ");
  TEST_ASSERT_EQUAL_STRING("sensor", buffer);
}

void test_truncates_to_the_rfc_limit() {
  char input[128];
  memset(input, 'a', sizeof(input) - 1);
  input[sizeof(input) - 1] = '\0';

  TEST_ASSERT_EQUAL_size_t(Hostname::MAX_LENGTH, sanitize(input));
  TEST_ASSERT_EQUAL_size_t(Hostname::MAX_LENGTH, strlen(buffer));
}

// Truncation must not leave the hyphen the limit fell on at the end
void test_truncation_does_not_leave_a_trailing_hyphen() {
  char input[80];
  memset(input, 'a', sizeof(input) - 1);
  input[sizeof(input) - 1] = '\0';
  input[Hostname::MAX_LENGTH - 1] = '-';

  const size_t length = sanitize(input);
  TEST_ASSERT_EQUAL_size_t(Hostname::MAX_LENGTH - 1, length);
  TEST_ASSERT_NOT_EQUAL('-', buffer[length - 1]);
}

void test_respects_a_buffer_smaller_than_the_limit() {
  char small[6];
  TEST_ASSERT_EQUAL_size_t(5, Hostname::sanitize("abcdefghij", small, sizeof(small)));
  TEST_ASSERT_EQUAL_STRING("abcde", small);
}

// Nothing usable left is the caller's cue to use its own default
void test_reports_an_empty_result() {
  TEST_ASSERT_EQUAL_size_t(0, sanitize("///"));
  TEST_ASSERT_EQUAL_STRING("", buffer);

  TEST_ASSERT_EQUAL_size_t(0, sanitize(""));
  TEST_ASSERT_EQUAL_size_t(0, sanitize(nullptr));
}

void test_survives_a_zero_sized_buffer() {
  TEST_ASSERT_EQUAL_size_t(0, Hostname::sanitize("sensor", buffer, 0));
  TEST_ASSERT_EQUAL_size_t(0, Hostname::sanitize("sensor", nullptr, sizeof(buffer)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_keeps_a_valid_label_as_is);
  RUN_TEST(test_folds_case_down);
  RUN_TEST(test_turns_separators_into_hyphens);
  RUN_TEST(test_collapses_repeated_separators);
  RUN_TEST(test_drops_unsupported_characters);
  RUN_TEST(test_strips_leading_and_trailing_hyphens);
  RUN_TEST(test_truncates_to_the_rfc_limit);
  RUN_TEST(test_truncation_does_not_leave_a_trailing_hyphen);
  RUN_TEST(test_respects_a_buffer_smaller_than_the_limit);
  RUN_TEST(test_reports_an_empty_result);
  RUN_TEST(test_survives_a_zero_sized_buffer);
  return UNITY_END();
}
