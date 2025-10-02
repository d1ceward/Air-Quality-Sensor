#pragma once

#include <U8g2lib.h>

namespace Display {
  void begin();
  void drawStrings(const String &line1, const String &line2 = "", const String &line3 = "");
}
