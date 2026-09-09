#include <string.h>
#include <unity.h>

#include "prom_text.h"

namespace {
  char buffer[64];

  size_t escape(const char *value) {
    return PromText::escapeLabelValue(value, buffer, sizeof(buffer));
  }
}

void setUp() {
  memset(buffer, 0, sizeof(buffer));
}

void tearDown() {}

void test_copies_a_plain_value() {
  TEST_ASSERT_EQUAL_size_t(11, escape("air-quality"));
  TEST_ASSERT_EQUAL_STRING("air-quality", buffer);
}

// An unescaped quote closes the label early and makes the whole scrape unparseable
void test_escapes_a_double_quote() {
  escape("salon \"nord\"");
  TEST_ASSERT_EQUAL_STRING("salon \\\"nord\\\"", buffer);
}

void test_escapes_a_backslash() {
  escape("a\\b");
  TEST_ASSERT_EQUAL_STRING("a\\\\b", buffer);
}

void test_escapes_a_line_feed() {
  escape("a\nb");
  TEST_ASSERT_EQUAL_STRING("a\\nb", buffer);
}

void test_escapes_every_occurrence() {
  TEST_ASSERT_EQUAL_size_t(6, escape("\"\"\""));
  TEST_ASSERT_EQUAL_STRING("\\\"\\\"\\\"", buffer);
}

void test_handles_an_empty_value() {
  TEST_ASSERT_EQUAL_size_t(0, escape(""));
  TEST_ASSERT_EQUAL_STRING("", buffer);
  TEST_ASSERT_EQUAL_size_t(0, escape(nullptr));
}

void test_reports_the_length_it_needed() {
  char small[4];
  // Three characters fit, the quote after them needs two more: five in total, two past the buffer
  TEST_ASSERT_EQUAL_size_t(5, PromText::escapeLabelValue("abc\"", small, sizeof(small)));
  TEST_ASSERT_EQUAL_STRING("abc", small);
}

// Half an escape sequence would corrupt the exposition, so the copy stops before it
void test_never_splits_an_escape_sequence() {
  char small[4];
  PromText::escapeLabelValue("a\"b", small, sizeof(small));
  TEST_ASSERT_EQUAL_STRING("a\\\"", small);

  char tight[3];
  PromText::escapeLabelValue("\"", tight, sizeof(tight));
  TEST_ASSERT_EQUAL_STRING("\\\"", tight);

  char tighter[2];
  PromText::escapeLabelValue("\"", tighter, sizeof(tighter));
  TEST_ASSERT_EQUAL_STRING("", tighter);
}

// A character that still fits after a dropped sequence must not be pulled forward
void test_truncation_keeps_the_order() {
  char small[4];
  PromText::escapeLabelValue("ab\"cd", small, sizeof(small));
  TEST_ASSERT_EQUAL_STRING("ab", small);
}

void test_survives_a_zero_sized_buffer() {
  TEST_ASSERT_EQUAL_size_t(3, PromText::escapeLabelValue("abc", buffer, 0));
  TEST_ASSERT_EQUAL_size_t(3, PromText::escapeLabelValue("abc", nullptr, sizeof(buffer)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_copies_a_plain_value);
  RUN_TEST(test_escapes_a_double_quote);
  RUN_TEST(test_escapes_a_backslash);
  RUN_TEST(test_escapes_a_line_feed);
  RUN_TEST(test_escapes_every_occurrence);
  RUN_TEST(test_handles_an_empty_value);
  RUN_TEST(test_reports_the_length_it_needed);
  RUN_TEST(test_never_splits_an_escape_sequence);
  RUN_TEST(test_truncation_keeps_the_order);
  RUN_TEST(test_survives_a_zero_sized_buffer);
  return UNITY_END();
}
