#include <U8g2lib.h>
#include <math.h>

// Local includes
#include "config.h"
#include "display_pages.h"
#include "net_time.h"
#include "net_wifi.h"

// Layout for the 64x48 SSD1306. Eight characters of the 8x16 font fill the panel exactly, so labels are
// abbreviated and values are right aligned.
namespace DisplayPages {
  namespace {
    constexpr int16_t PANEL_WIDTH = 64;
    constexpr int16_t PANEL_HEIGHT = 48;
    constexpr uint8_t SMALL_LINE_HEIGHT = 8;

    String valueOrDash(bool valid, uint16_t value) {
      return valid ? String(value) : String("--");
    }

    void drawRight(U8G2 &oled, int16_t y, const String &text) {
      oled.drawUTF8(PANEL_WIDTH - 1 - oled.getUTF8Width(text.c_str()), y, text.c_str());
    }

    void drawCentered(U8G2 &oled, int16_t y, const String &text) {
      oled.drawUTF8((PANEL_WIDTH - oled.getUTF8Width(text.c_str())) / 2, y, text.c_str());
    }

    // Breaks text on spaces to fit the panel width, returns the next baseline
    int16_t drawWrapped(U8G2 &oled, int16_t y, const String &text) {
      String line;

      for (int16_t start = 0; start <= static_cast<int16_t>(text.length());) {
        int16_t space = text.indexOf(' ', start);
        if (space < 0)
          space = text.length();

        const String word = text.substring(start, space);
        const String candidate = line.length() == 0 ? word : line + " " + word;

        if (oled.getUTF8Width(candidate.c_str()) <= PANEL_WIDTH) {
          line = candidate;
        } else {
          if (y <= PANEL_HEIGHT)
            oled.drawUTF8(0, y, line.c_str());

          y += SMALL_LINE_HEIGHT;
          line = word;
        }

        start = space + 1;
      }

      if (line.length() > 0 && y <= PANEL_HEIGHT) {
        oled.drawUTF8(0, y, line.c_str());
        y += SMALL_LINE_HEIGHT;
      }

      return y;
    }

    void drawSensors(U8G2 &oled, const Sensors::Snapshot &readings) {
      oled.clearBuffer();

      oled.setFont(u8g2_font_t0_16_tf);
      oled.drawStr(0, 12, "CO2");
      drawRight(oled, 12, valueOrDash(readings.hasCo2, readings.co2Ppm));

      oled.drawStr(0, 30, "PM25");
      drawRight(oled, 30, valueOrDash(readings.hasParticulate, readings.particulate.pm25));

      // The third line carries two values, so it uses a narrower font and a whole number: "-10°C" next to
      // "100%" already fills the panel.
      oled.setFont(u8g2_font_6x10_tf);
      if (readings.hasClimate) {
        oled.drawUTF8(0, 46, (String(static_cast<int>(lroundf(readings.temperatureC))) + "°C").c_str());
        drawRight(oled, 46, String(static_cast<int>(readings.humidity + 0.5f)) + "%");
      } else {
        oled.drawStr(0, 46, "--");
      }

      oled.sendBuffer();
    }

    void drawClock(U8G2 &oled) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_6x10_tf);
      drawCentered(oled, 12, NetTime::formattedWeekday() + " " + NetTime::formattedYear());
      oled.setFont(u8g2_font_t0_16_tf);
      drawCentered(oled, 30, NetTime::formattedDate());
      drawCentered(oled, 46, NetTime::formattedClock());
      oled.sendBuffer();
    }

    void drawSetup(U8G2 &oled) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_5x7_tf);

      int16_t y = drawWrapped(oled, 7, "Setup");
      y = drawWrapped(oled, y, NetWifi::accessPointSsid());
      y = drawWrapped(oled, y, String("pass ") + SETUP_AP_PASSWORD);
      drawWrapped(oled, y, NetWifi::ipAddress());

      oled.sendBuffer();
    }

    void drawMessage(U8G2 &oled, const String &line1, const String &line2, const String &line3) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_5x7_tf);

      int16_t y = drawWrapped(oled, 8, line1);
      y = drawWrapped(oled, y, line2);
      drawWrapped(oled, y, line3);

      oled.sendBuffer();
    }
  }

  const Layout COMPACT_64X48 = {drawSensors, drawClock, drawSetup, drawMessage};
}
