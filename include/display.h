#pragma once

/**
 * Display abstraction layer.
 */

#include <U8g2lib.h>

namespace Display {
  void begin();
  void showSplash();
  void showStatus(const String &line1, const String &line2 = "", const String &line3 = "");
  void loop();
} // namespace Display
