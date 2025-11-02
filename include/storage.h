#pragma once

#include <Arduino.h>

namespace Storage {
  void begin();
  void writeString(int addr, const String& value, size_t maxLen);
  String readString(int addr, size_t maxLen);
  void commit();
}
