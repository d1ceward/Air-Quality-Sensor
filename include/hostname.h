#pragma once

#include <stddef.h>

// Sanitises the configured device id into a DNS label usable for DHCP and mDNS.
//
// Free of Arduino dependencies so it can be exercised on the host.
namespace Hostname {
  // RFC 1123 caps a label at 63 characters
  constexpr size_t MAX_LENGTH = 63;

  // Writes an RFC 1123 label into buffer: letters, digits and hyphens only, no leading or trailing hyphen,
  // truncated to MAX_LENGTH. Spaces and underscores become hyphens, anything else is dropped, and upper case
  // is folded down because DNS comparisons ignore case and mixed case only confuses the logs.
  //
  // Always NUL terminates when bufferSize is not zero. Returns the length written, zero when nothing usable
  // was left, which is the caller's cue to fall back to its own default.
  size_t sanitize(const char *value, char *buffer, size_t bufferSize);
}
