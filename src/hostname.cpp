#include "hostname.h"

namespace Hostname {
  namespace {
    bool isAlphaNumericAscii(char character) {
      return (character >= '0' && character <= '9') ||
             (character >= 'a' && character <= 'z') ||
             (character >= 'A' && character <= 'Z');
    }

    char toLowerAscii(char character) {
      return (character >= 'A' && character <= 'Z') ? static_cast<char>(character + ('a' - 'A'))
                                                    : character;
    }
  }

  size_t sanitize(const char *value, char *buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0)
      return 0;

    buffer[0] = '\0';
    if (value == nullptr)
      return 0;

    // One character of the buffer goes to the terminator
    size_t limit = bufferSize - 1;
    if (limit > MAX_LENGTH)
      limit = MAX_LENGTH;

    size_t length = 0;

    for (const char *cursor = value; *cursor != '\0' && length < limit; ++cursor) {
      char character = *cursor;

      if (character == ' ' || character == '_' || character == '.')
        character = '-';
      else if (!isAlphaNumericAscii(character) && character != '-')
        continue;

      // A label may not start with a hyphen, and a run of separators must not become a run of them
      if (character == '-' && (length == 0 || buffer[length - 1] == '-'))
        continue;

      buffer[length++] = toLowerAscii(character);
    }

    // Nor may it end with one
    while (length > 0 && buffer[length - 1] == '-')
      --length;

    buffer[length] = '\0';

    return length;
  }
}
