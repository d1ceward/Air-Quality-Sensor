#include <string.h>
#include <unity.h>

#include "log_ring.h"

static LogRing::Buffer ring;

void setUp() {
  ring.clear();
}

void tearDown() {}

static void writeText(const char *text) {
  while (*text)
    ring.write(static_cast<uint8_t>(*text++));
}

void test_reads_back_what_was_written() {
  writeText("boot\n");

  char out[16] = {};
  TEST_ASSERT_EQUAL_UINT32(5, ring.total());
  TEST_ASSERT_EQUAL(5, ring.read(0, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("boot\n", out);
}

// A client polling with the offset it was last given only gets the new bytes
void test_reads_from_an_offset() {
  writeText("first second");

  char out[16] = {};
  TEST_ASSERT_EQUAL(6, ring.read(6, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("second", out);
}

void test_nothing_new_reads_nothing() {
  writeText("abc");

  char out[4];
  TEST_ASSERT_EQUAL(0, ring.read(3, out, sizeof(out)));
}

void test_read_is_capped_by_the_output_size() {
  writeText("abcdef");

  char out[4] = {};
  TEST_ASSERT_EQUAL(3, ring.read(0, out, 3));
  TEST_ASSERT_EQUAL_STRING("abc", out);
}

// Once full, the oldest bytes go first and the offsets keep counting from boot
void test_keeps_the_most_recent_bytes_once_full() {
  for (size_t i = 0; i < LogRing::CAPACITY + 10; ++i)
    ring.write(static_cast<uint8_t>('a' + i % 26));

  TEST_ASSERT_EQUAL_UINT32(LogRing::CAPACITY + 10, ring.total());
  TEST_ASSERT_EQUAL_UINT32(10, ring.oldest());

  char out[1];
  TEST_ASSERT_EQUAL(1, ring.read(10, out, 1));
  TEST_ASSERT_EQUAL_CHAR('a' + 10 % 26, out[0]);

  TEST_ASSERT_EQUAL(1, ring.read(LogRing::CAPACITY + 9, out, 1));
  TEST_ASSERT_EQUAL_CHAR('a' + (LogRing::CAPACITY + 9) % 26, out[0]);
}

// An offset that already fell out of the window starts at the oldest byte still held
void test_clamps_an_offset_that_was_overwritten() {
  for (size_t i = 0; i < LogRing::CAPACITY + 100; ++i)
    ring.write('x');

  TEST_ASSERT_EQUAL_UINT32(100, ring.clamp(5));
  TEST_ASSERT_EQUAL_UINT32(150, ring.clamp(150));
}

// A client that last polled before a reboot holds an offset past the end, and is sent everything
void test_clamps_an_offset_past_the_end() {
  writeText("after reboot");

  TEST_ASSERT_EQUAL_UINT32(0, ring.clamp(5000));

  char out[16] = {};
  TEST_ASSERT_EQUAL(12, ring.read(5000, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("after reboot", out);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reads_back_what_was_written);
  RUN_TEST(test_reads_from_an_offset);
  RUN_TEST(test_nothing_new_reads_nothing);
  RUN_TEST(test_read_is_capped_by_the_output_size);
  RUN_TEST(test_keeps_the_most_recent_bytes_once_full);
  RUN_TEST(test_clamps_an_offset_that_was_overwritten);
  RUN_TEST(test_clamps_an_offset_past_the_end);
  return UNITY_END();
}
