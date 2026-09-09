#include "prom_text.h"

namespace PromText {
  namespace {
    // Returns the two character escape for the given byte, or nullptr when it is copied as is
    const char *escapeFor(char character) {
      switch (character) {
        case '\\': return "\\\\";
        case '"': return "\\\"";
        case '\n': return "\\n";
        default: return nullptr;
      }
    }
  }

  size_t escapeLabelValue(const char *value, char *buffer, size_t bufferSize) {
    if (buffer != nullptr && bufferSize > 0)
      buffer[0] = '\0';

    if (value == nullptr)
      return 0;

    // One character of the buffer goes to the terminator
    const size_t limit = (buffer != nullptr && bufferSize > 0) ? bufferSize - 1 : 0;
    size_t needed = 0;
    size_t written = 0;
    bool truncated = false;

    for (const char *cursor = value; *cursor != '\0'; ++cursor) {
      const char *escape = escapeFor(*cursor);
      const size_t width = escape == nullptr ? 1 : 2;

      needed += width;

      // Once the buffer is out of room the copy stops for good: emitting half a sequence would corrupt the
      // scrape, and picking up a later character that happens to fit would reorder the value. The count
      // keeps running so the caller still learns the size it needed.
      if (truncated || written + width > limit) {
        truncated = true;
        continue;
      }

      if (escape == nullptr) {
        buffer[written++] = *cursor;
      } else {
        buffer[written++] = escape[0];
        buffer[written++] = escape[1];
      }
    }

    if (limit > 0)
      buffer[written] = '\0';

    return needed;
  }
}
