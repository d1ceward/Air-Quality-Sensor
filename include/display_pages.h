#pragma once

#include <U8g2lib.h>

// Local includes
#include "sensors.h"

// The firmware drives two panels with very different room: a 128x64 SH1106 and a 64x48 SSD1306. Each geometry
// gets its own renderer, and the display module picks one at boot from the configuration.
namespace DisplayPages {
  struct Layout {
    void (*drawSensors)(U8G2 &oled, const Sensors::Snapshot &readings);
    void (*drawClock)(U8G2 &oled);
    void (*drawSetup)(U8G2 &oled);
    void (*drawMessage)(U8G2 &oled, const String &line1, const String &line2, const String &line3);
  };

  extern const Layout WIDE_128X64;
  extern const Layout COMPACT_64X48;
}
