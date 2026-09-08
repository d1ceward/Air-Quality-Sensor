#include <U8g2lib.h>
#include <math.h>

// Local includes
#include "battery.h"
#include "config.h"
#include "display_pages.h"
#include "net_time.h"
#include "net_wifi.h"

// Layout for the 128x64 SH1106: every reading fits on one page, in three columns under a temperature and
// humidity header.
namespace DisplayPages {
  namespace {
    String valueOrDash(bool valid, uint16_t value) {
      return valid ? String(value) : String("--");
    }

    void drawSensors(U8G2 &oled, const Sensors::Snapshot &readings) {
      oled.clearBuffer();

      oled.drawLine(2, 14, 126, 14);
      oled.drawLine(41, 14, 41, 64);
      oled.drawLine(86, 14, 86, 64);

      oled.setFont(u8g2_font_t0_16_tf);
      if (readings.hasClimate) {
        oled.drawUTF8(2, 10, (String(readings.temperatureC, 1) + "°C").c_str());
        oled.drawStr(96, 10, (String(static_cast<int>(readings.humidity + 0.5f)) + "%").c_str());
      } else {
        oled.drawStr(2, 10, "--");
      }

      // Pressure fits in the gap left between temperature and humidity. It starts at 60 so a negative two
      // digit temperature cannot run into it.
      if (readings.hasPressure) {
        oled.setFont(u8g2_font_5x7_tf);
        oled.drawStr(60, 9, (String(lroundf(readings.pressureHpa)) + "hPa").c_str());
      }

      oled.setFont(u8g2_font_t0_16_tf);
      oled.drawStr(2, 47, valueOrDash(readings.hasCo2, readings.co2Ppm).c_str());
      oled.drawStr(46, 47, valueOrDash(readings.hasParticulate, readings.particulate.pm25).c_str());
      oled.drawStr(91, 47, valueOrDash(readings.hasTvoc, readings.tvocPpb).c_str());

      oled.setFont(u8g2_font_t0_14_tf);
      oled.drawStr(2, 30, "CO2");
      oled.drawStr(2, 62, "ppm");
      oled.drawStr(46, 30, "PM2.5");
      oled.drawUTF8(46, 62, "ug/m³");
      oled.drawStr(91, 30, "TVOC");
      oled.drawStr(91, 62, "ppb");

      oled.sendBuffer();
    }

    // A 20x9 battery outline filled in proportion to the charge
    void drawBatteryGauge(U8G2 &oled, int16_t x, int16_t y) {
      constexpr int16_t BODY_WIDTH = 18;
      constexpr int16_t BODY_HEIGHT = 9;

      oled.drawFrame(x, y, BODY_WIDTH, BODY_HEIGHT);
      oled.drawBox(x + BODY_WIDTH, y + 3, 2, 3);

      const int16_t fill = (BODY_WIDTH - 4) * Battery::percent() / 100;
      if (fill > 0)
        oled.drawBox(x + 2, y + 2, fill, BODY_HEIGHT - 4);
    }

    void drawClock(U8G2 &oled) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_helvB18_tn);
      oled.drawStr(11, 21, NetTime::formattedDate().c_str());
      oled.setFont(u8g2_font_helvB10_tr);
      oled.drawStr(77, 12, NetTime::formattedYear().c_str());
      oled.drawStr(77, 26, NetTime::formattedWeekday().c_str());
      oled.setFont(u8g2_font_helvB24_tn);
      oled.drawStr(16, 56, NetTime::formattedClock().c_str());

      // The clock page is the only one with room to spare
      if (Battery::hasReading()) {
        drawBatteryGauge(oled, 104, 38);
        oled.setFont(u8g2_font_5x7_tf);
        oled.drawStr(104, 58, (String(Battery::percent()) + "%").c_str());
      }

      oled.sendBuffer();
    }

    void drawSetup(U8G2 &oled) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_t0_14_tf);
      oled.drawStr(2, 12, "WiFi setup");
      oled.setFont(u8g2_font_5x7_tf);
      oled.drawStr(2, 28, NetWifi::accessPointSsid().c_str());
      oled.drawStr(2, 40, (String("pass ") + SETUP_AP_PASSWORD).c_str());
      oled.drawStr(2, 56, (String("http://") + NetWifi::ipAddress()).c_str());
      oled.sendBuffer();
    }

    void drawMessage(U8G2 &oled, const String &line1, const String &line2, const String &line3) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_t0_16_tf);
      oled.drawUTF8(1, 16, line1.c_str());
      oled.drawUTF8(1, 36, line2.c_str());
      oled.drawUTF8(1, 56, line3.c_str());
      oled.sendBuffer();
    }
  }

  const Layout WIDE_128X64 = {drawSensors, drawClock, drawSetup, drawMessage};
}
