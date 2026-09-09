#pragma once

#include <stddef.h>

// Prometheus text exposition format helpers.
//
// Free of Arduino dependencies so they can be exercised on the host.
namespace PromText {
  // Escapes a label value: a backslash, a double quote and a line feed each become a two character
  // sequence, everything else is copied through. An unescaped quote in a device id would otherwise close
  // the label early and make the whole scrape unparseable.
  //
  // Writes at most bufferSize - 1 characters and always NUL terminates when bufferSize is not zero,
  // truncating between two sequences rather than in the middle of one. Returns the length the full result
  // would have had, excluding the terminator, so a return value of bufferSize or more means it was
  // truncated.
  size_t escapeLabelValue(const char *value, char *buffer, size_t bufferSize);
}
