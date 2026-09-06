#pragma once

#include <Arduino.h>

// 128x64 OLED (SH1106) driven over hardware I2C.
namespace Display {
  void begin();

  // Rotates between the sensor page and the clock (or setup) page.
  void loop();

  // Immediately shows a status message, used during boot and OTA updates.
  void showMessage(const String &line1, const String &line2 = "", const String &line3 = "");
}
